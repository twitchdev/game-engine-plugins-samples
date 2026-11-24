#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "TwitchSDK.h"

#include <exception>
#include <memory>

DECLARE_LOG_CATEGORY_EXTERN(LogTwitchSample, Log, All);

/**
 * An owned copy of Text for API request structs. Assigning a string_view to a string_holder only
 * stores a view, which would dangle once the FString goes away.
 */
inline TwitchSDK::string_holder ToOwned(const FString& Text)
{
	return TwitchSDK::string_holder(TwitchSDK::string(TwitchSDK::FromFString(Text)));
}

inline std::vector<TwitchSDK::string_holder> ToOwned(const TArray<FString>& Items)
{
	std::vector<TwitchSDK::string_holder> Result;
	Result.reserve(Items.Num());
	for (const FString& Item : Items)
		Result.push_back(ToOwned(Item));
	return Result;
}

enum class ESampleLogLevel : uint8
{
	Info,
	Success,
	Warning,
	Error,
};

struct FSampleLogEntry
{
	FDateTime Time;
	ESampleLogLevel Level = ESampleLogLevel::Info;
	FString Message;
};

/**
 * State shared by every page of the sample: access to the plugin's API, the login state, which
 * calls are running, and the log. Use it from the game thread only.
 *
 * The core library reports results through resolve/reject callbacks that can run on any thread
 * (the plugin does file I/O on the thread pool, for example). Call() copies each result and
 * delivers it on the game thread, so pages can update their widgets directly.
 */
class FTwitchSampleContext : public TSharedFromThis<FTwitchSampleContext>
{
public:
	using FApi = TwitchSDK::TwitchSDKApi;

	~FTwitchSampleContext();

	/** Applies the saved Client ID and starts checking the login state once a second. */
	void Start();

	/** Stops the login state checks. Results that arrive later are ignored. */
	void Shutdown();

	/** The plugin's API instance, or null if the plugin isn't loaded. */
	std::shared_ptr<FApi> GetApi() const;

	/** The TwitchSDK plugin version, or "not loaded". */
	FString GetPluginVersion() const;

	// ---------------------------------------------------------------- Client ID

	/** The Client ID saved by the sample, falling back to the one in Project Settings. */
	FString GetSavedClientId() const;

	/** The Client ID the API is currently using (empty until one is applied). */
	const FString& GetAppliedClientId() const { return AppliedClientId; }
	bool HasClientId() const { return !AppliedClientId.IsEmpty(); }

	/** Switches the API to a new Client ID and saves it for next time. */
	void ApplyClientId(const FString& ClientId);

	/** Whether the plugin connects to a local EventSubProxy (set in Project Settings). */
	bool UsesEventSubProxy() const;

	// ---------------------------------------------------------------- Login state

	bool HasAuthState() const { return bHasAuthState; }
	TwitchSDK::AuthStatus GetAuthStatus() const { return AuthStatus; }
	const TArray<FString>& GetGrantedScopes() const { return GrantedScopes; }
	bool IsLoggedIn() const { return bHasAuthState && AuthStatus == TwitchSDK::AuthStatus::LoggedIn; }
	FText GetStatusText() const;

	/** Re-reads the login state right away instead of waiting for the next check. */
	void RefreshAuthState();

	/** Fired on the game thread when the login status changes. */
	FSimpleMulticastDelegate OnAuthStateChanged;

	// ---------------------------------------------------------------- Log

	void Log(ESampleLogLevel Level, const FString& Message);
	void Info(const FString& Message) { Log(ESampleLogLevel::Info, Message); }
	void Success(const FString& Message) { Log(ESampleLogLevel::Success, Message); }
	void Warning(const FString& Message) { Log(ESampleLogLevel::Warning, Message); }
	void Error(const FString& Message) { Log(ESampleLogLevel::Error, Message); }
	void ClearLog();

	/** Log entries, newest first. */
	const TArray<FSampleLogEntry>& GetLogEntries() const { return LogEntries; }

	/** Fired on the game thread whenever the log changes. */
	FSimpleMulticastDelegate OnLogChanged;

	// ---------------------------------------------------------------- API calls

	/** Whether the call started under this action name is still running. */
	bool IsRunning(const FString& Action) const { return Running.Contains(Action); }

	/**
	 * Calls an API method and delivers the result on the game thread.
	 *
	 * @param Action     Name shown in errors. While the call runs, IsRunning(Action) is true and
	 *                   repeated calls with the same name are ignored (unless bTrackAsRunning is false).
	 * @param Invoke     Starts the call, e.g. [](FApi& Api, auto Resolve, auto Reject) { Api.GetMyUserInfo(Resolve, Reject); }
	 * @param OnSuccess  Runs on the game thread with an owned copy of the result.
	 * @param OnError    Optional. Runs on the game thread with the error message instead of logging it.
	 */
	template <class TResult>
	void Call(
		const FString& Action,
		TFunction<void(FApi&, TwitchSDK::ResolveFn<const TResult&>, TwitchSDK::RejectFn)> Invoke,
		TFunction<void(const TResult&)> OnSuccess,
		TFunction<void(const FString&)> OnError = nullptr,
		bool bTrackAsRunning = true);

	/** Like Call, for API methods that don't return a value. */
	void CallVoid(
		const FString& Action,
		TFunction<void(FApi&, TwitchSDK::ResolveFn<void>, TwitchSDK::RejectFn)> Invoke,
		TFunction<void()> OnSuccess,
		TFunction<void(const FString&)> OnError = nullptr,
		bool bTrackAsRunning = true);

	/** Queues Fn to run on the game thread, even when called from the game thread. */
	static void RunOnGameThread(TFunction<void()> Fn);

private:
	bool BeginCall(const FString& Action, bool bTrackAsRunning);
	void FinishCall(const FString& Action, bool bTrackAsRunning);
	void ReportError(const FString& Action, const FString& Message, const TFunction<void(const FString&)>& OnError);
	bool TickAuthState(float DeltaTime);

	FString AppliedClientId;

	bool bHasAuthState = false;
	TwitchSDK::AuthStatus AuthStatus = TwitchSDK::AuthStatus::LoggedOut;
	TArray<FString> GrantedScopes;
	FTSTicker::FDelegateHandle AuthTicker;

	TSet<FString> Running;
	TArray<FSampleLogEntry> LogEntries;
};

// --------------------------------------------------------------------------- template implementation

template <class TResult>
void FTwitchSampleContext::Call(
	const FString& Action,
	TFunction<void(FApi&, TwitchSDK::ResolveFn<const TResult&>, TwitchSDK::RejectFn)> Invoke,
	TFunction<void(const TResult&)> OnSuccess,
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

	TwitchSDK::ResolveFn<const TResult&> Resolve = [WeakThis, Action, bTrackAsRunning, OnSuccess](const TResult& Result)
	{
		// Copying a result makes every string in it owned, so it's safe to keep after the callback.
		TResult Owned = Result;
		RunOnGameThread([WeakThis, Action, bTrackAsRunning, OnSuccess, Owned]()
		{
			if (const TSharedPtr<FTwitchSampleContext> This = WeakThis.Pin())
			{
				This->FinishCall(Action, bTrackAsRunning);
				if (OnSuccess)
					OnSuccess(Owned);
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
