#include "UI/TwitchSampleContext.h"

#include "Async/Async.h"
#include "Misc/ConfigCacheIni.h"
#include "TwitchSDKSettings.h"

DEFINE_LOG_CATEGORY(LogTwitchSample);

namespace
{
	// Where earlier versions of this sample saved the Client ID (read as a fallback only).
	const TCHAR* ConfigSection = TEXT("TwitchCppSample");
	const TCHAR* ConfigClientIdKey = TEXT("ClientId");
	constexpr int32 MaxLogEntries = 300;
}

FTwitchSampleContext::~FTwitchSampleContext()
{
	Shutdown();
}

void FTwitchSampleContext::Start()
{
	if (!GetApi())
	{
		Error(TEXT("The TwitchSDK plugin isn't loaded. Run import-plugin.bat and rebuild the project."));
		return;
	}

	Info(FString::Printf(TEXT("TwitchSDK plugin %s loaded."), *GetPluginVersion()));

	// The plugin creates its API at startup with the Client ID from Project Settings. Switch it to
	// the one saved by the sample (if any) so a previous session's login keeps working.
	const FString Saved = GetSavedClientId();
	if (!Saved.IsEmpty())
		ApplyClientId(Saved);

	AuthTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateSP(this, &FTwitchSampleContext::TickAuthState), 1.0f);
	RefreshAuthState();
}

void FTwitchSampleContext::Shutdown()
{
	if (AuthTicker.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(AuthTicker);
		AuthTicker.Reset();
	}
	Running.Empty();
}

std::shared_ptr<TwitchSDK::TwitchSDKApi> FTwitchSampleContext::GetApi() const
{
	FTwitchSDKModule* Module = FModuleManager::GetModulePtr<FTwitchSDKModule>(TEXT("TwitchSDK"));
	return Module ? Module->Core : nullptr;
}

FString FTwitchSampleContext::GetPluginVersion() const
{
	// TWITCHSDK_VERSION is defined by the plugin's Build.cs from core/VERSION, the single source
	// of truth for the plugin version (TwitchSDK::Version itself isn't exported from the DLL).
	if (!GetApi())
		return TEXT("not loaded");
	return TEXT(TWITCHSDK_VERSION);
}

// ---------------------------------------------------------------------------- Client ID

FString FTwitchSampleContext::GetSavedClientId() const
{
	// The plugin's own Client ID setting (Project Settings, or the per-user value ApplyClientId saves).
	const UTwitchSDKSettings* Settings = GetDefault<UTwitchSDKSettings>();
	if (Settings && !Settings->ClientId.IsEmpty())
		return Settings->ClientId;

	// Fallback: earlier versions of this sample saved it under its own key instead.
	FString Legacy;
	if (GConfig && GConfig->GetString(ConfigSection, ConfigClientIdKey, Legacy, GGameUserSettingsIni))
		return Legacy;
	return FString();
}

void FTwitchSampleContext::ApplyClientId(const FString& ClientId)
{
	const std::shared_ptr<FApi> Api = GetApi();
	const FString Trimmed = ClientId.TrimStartAndEnd();
	if (!Api || Trimmed.IsEmpty())
		return;

	const bool bChanged = !AppliedClientId.IsEmpty() && AppliedClientId != Trimmed;

	// UpdateOAuthCredentials is the plugin's way to switch Client IDs during development.
	Api->UpdateOAuthCredentials(TwitchSDK::FromFString(Trimmed));
	AppliedClientId = Trimmed;

	// Save it as the plugin's own setting, not just for the sample. The plugin starts its API when
	// the engine starts, long before play begins. With a saved login but no Client ID, its first
	// request fails with 401, and the token refresh that follows (also without a Client ID) makes
	// the core delete the saved login.
	//
	// The value goes into the plugin's section of the per-user Saved/Config/<Platform>/Game.ini,
	// which overrides Config/DefaultGame.ini without changing it. UE only saves Game.ini sections
	// listed under [SectionsToSave], so the sample's DefaultGame.ini allows this one. Flushing right
	// away keeps it even if the game doesn't exit cleanly.
	UTwitchSDKSettings* Settings = GetMutableDefault<UTwitchSDKSettings>();
	if (Settings && GConfig && Settings->ClientId != Trimmed)
	{
		Settings->ClientId = Trimmed;
		GConfig->SetString(TEXT("/Script/TwitchSDK.TwitchSDKSettings"), TEXT("ClientId"), *Trimmed, GGameIni);
		GConfig->Flush(false, GGameIni);
	}

	Success(TEXT("Using your Client ID. Now log in below."));
	if (bChanged && IsLoggedIn())
		Warning(TEXT("You're still logged in from the previous Client ID. Log out and log in again to use the new one."));
}

bool FTwitchSampleContext::UsesEventSubProxy() const
{
	const UTwitchSDKSettings* Settings = GetDefault<UTwitchSDKSettings>();
	return Settings && Settings->UseEventSubProxy;
}

// ---------------------------------------------------------------------------- Login state

FText FTwitchSampleContext::GetStatusText() const
{
	if (!GetApi())
		return FText::FromString(TEXT("Plugin not loaded"));
	if (!HasClientId())
		return FText::FromString(TEXT("Enter your Client ID"));
	if (!bHasAuthState)
		return FText::FromString(TEXT("Checking login ..."));

	switch (AuthStatus)
	{
	case TwitchSDK::AuthStatus::LoggedIn: return FText::FromString(TEXT("Logged in"));
	case TwitchSDK::AuthStatus::WaitingForCode: return FText::FromString(TEXT("Waiting for you to enter the code on Twitch"));
	case TwitchSDK::AuthStatus::Loading: return FText::FromString(TEXT("Loading ..."));
	default: return FText::FromString(TEXT("Logged out"));
	}
}

void FTwitchSampleContext::RefreshAuthState()
{
	TickAuthState(0.f);
}

bool FTwitchSampleContext::TickAuthState(float)
{
	const std::shared_ptr<FApi> Api = GetApi();
	if (!Api)
		return true;

	// GetAuthState returns immediately; this wrapper reads it synchronously.
	const TwitchSDK::AuthState State = Api->GetAuthState();

	TArray<FString> Scopes;
	for (const TwitchSDK::string_holder& Scope : State.Scopes)
		Scopes.Add(TwitchSDK::ToFString(Scope));

	const bool bHadState = bHasAuthState;
	const TwitchSDK::AuthStatus Previous = AuthStatus;
	bHasAuthState = true;
	AuthStatus = State.Status;
	GrantedScopes = MoveTemp(Scopes);

	if (!bHadState || Previous != AuthStatus)
	{
		if (AuthStatus == TwitchSDK::AuthStatus::LoggedIn)
			bHadState ? Success(TEXT("Logged in to Twitch.")) : Info(TEXT("Already logged in (using the saved token)."));
		else if (bHadState && Previous == TwitchSDK::AuthStatus::LoggedIn)
			Info(TEXT("Logged out."));

		OnAuthStateChanged.Broadcast();
	}
	return true; // keep ticking
}

// ---------------------------------------------------------------------------- Log

void FTwitchSampleContext::Log(ESampleLogLevel Level, const FString& Message)
{
	FSampleLogEntry Entry;
	Entry.Time = FDateTime::Now();
	Entry.Level = Level;
	Entry.Message = Message;
	LogEntries.Insert(MoveTemp(Entry), 0);
	if (LogEntries.Num() > MaxLogEntries)
		LogEntries.SetNum(MaxLogEntries);

	switch (Level)
	{
	case ESampleLogLevel::Error: UE_LOG(LogTwitchSample, Error, TEXT("%s"), *Message); break;
	case ESampleLogLevel::Warning: UE_LOG(LogTwitchSample, Warning, TEXT("%s"), *Message); break;
	default: UE_LOG(LogTwitchSample, Log, TEXT("%s"), *Message); break;
	}

	OnLogChanged.Broadcast();
}

void FTwitchSampleContext::ClearLog()
{
	LogEntries.Reset();
	OnLogChanged.Broadcast();
}

// ---------------------------------------------------------------------------- API calls

void FTwitchSampleContext::CallVoid(
	const FString& Action,
	TFunction<void(FApi&, TwitchSDK::ResolveFn<void>, TwitchSDK::RejectFn)> Invoke,
	TFunction<void()> OnSuccess,
	TFunction<void(const FString&)> OnError,
	bool bTrackAsRunning)
{
	const std::shared_ptr<FApi> Api = GetApi();
	if (!Api)
	{
		ReportError(Action, TEXT("the Twitch plugin isn't loaded"), OnError);
		return;
	}
	if (!BeginCall(Action, bTrackAsRunning))
		return;

	const TWeakPtr<FTwitchSampleContext> WeakThis = AsShared();

	TwitchSDK::ResolveFn<void> Resolve = [WeakThis, Action, bTrackAsRunning, OnSuccess]()
	{
		RunOnGameThread([WeakThis, Action, bTrackAsRunning, OnSuccess]()
		{
			if (const TSharedPtr<FTwitchSampleContext> This = WeakThis.Pin())
			{
				This->FinishCall(Action, bTrackAsRunning);
				if (OnSuccess)
					OnSuccess();
			}
		});
	};

	TwitchSDK::RejectFn Reject = [WeakThis, Action, bTrackAsRunning, OnError](const std::exception& Exception)
	{
		const FString Message = UTF8_TO_TCHAR(Exception.what());
		RunOnGameThread([WeakThis, Action, bTrackAsRunning, OnError, Message]()
		{
			if (const TSharedPtr<FTwitchSampleContext> This = WeakThis.Pin())
			{
				This->FinishCall(Action, bTrackAsRunning);
				This->ReportError(Action, Message, OnError);
			}
		});
	};

	try
	{
		Invoke(*Api, Resolve, Reject);
	}
	catch (const std::exception& Exception)
	{
		Reject(Exception);
	}
}

void FTwitchSampleContext::RunOnGameThread(TFunction<void()> Fn)
{
	// Always queue, even on the game thread: some calls resolve synchronously from inside the
	// API, and handling the result later keeps the order of events predictable.
	AsyncTask(ENamedThreads::GameThread, MoveTemp(Fn));
}

bool FTwitchSampleContext::BeginCall(const FString& Action, bool bTrackAsRunning)
{
	if (!bTrackAsRunning)
		return true;
	if (Running.Contains(Action))
		return false; // ignore repeat clicks while running
	Running.Add(Action);
	return true;
}

void FTwitchSampleContext::FinishCall(const FString& Action, bool bTrackAsRunning)
{
	if (bTrackAsRunning)
		Running.Remove(Action);
}

void FTwitchSampleContext::ReportError(const FString& Action, const FString& Message, const TFunction<void(const FString&)>& OnError)
{
	if (OnError)
		OnError(Message);
	else
		Error(FString::Printf(TEXT("%s failed: %s"), *Action, *Message));
}
