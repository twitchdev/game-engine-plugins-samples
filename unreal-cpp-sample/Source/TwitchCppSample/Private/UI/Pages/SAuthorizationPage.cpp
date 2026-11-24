#include "UI/Pages/SAuthorizationPage.h"

#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/SampleWidgets.h"

using TwitchSDK::ToFString;
using TwitchSDK::FromFString;

void SAuthorizationPage::Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext)
{
	Context = InContext;
	ClientIdText = Context->GetSavedClientId();

	// Every scope the other pages need. Drops and raid events need no scope.
	Scopes = {
		{ TEXT("channel:manage:polls"), TEXT("Polls page") },
		{ TEXT("channel:manage:predictions"), TEXT("Predictions page") },
		{ TEXT("channel:manage:broadcast"), TEXT("Edit channel info, create stream markers") },
		{ TEXT("clips:edit"), TEXT("Create clips") },
		{ TEXT("channel:manage:redemptions"), TEXT("Channel point rewards and redemption events") },
		{ TEXT("user:read:subscriptions"), TEXT("Check whether you subscribe to a channel") },
		{ TEXT("channel:read:subscriptions"), TEXT("Subscription events") },
		{ TEXT("moderator:read:followers"), TEXT("Follow events") },
		{ TEXT("bits:read"), TEXT("Cheer events and the Bits leaderboard") },
		{ TEXT("channel:read:hype_train"), TEXT("Hype Train events") },
		{ TEXT("user:read:email"), TEXT("Email address in your user info") },
	};

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeClientIdSection() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeScopesSection() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeLoginSection() ]
	];
}

void SAuthorizationPage::ReleaseApiResources()
{
	UserCode.Reset();
	ActivationUri.Reset();
}

TSharedRef<SWidget> SAuthorizationPage::MakeClientIdSection()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("1. Client ID"),
				TEXT("Register an application in the Twitch developer console. Set the OAuth Redirect URL to http://localhost and "
					 "the Client Type to Public (the device code login used here needs a public client). Then paste its Client ID below."))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Client ID"), &ClientIdText) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					SampleUI::Button(TEXT("Use this Client ID"),
						[this]() { Context->ApplyClientId(ClientIdText); Context->RefreshAuthState(); },
						TAttribute<bool>::CreateLambda([this]() { return !ClientIdText.TrimStartAndEnd().IsEmpty(); }))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SampleUI::Button(TEXT("Open the Twitch developer console"),
						[]() { FPlatformProcess::LaunchURL(TEXT("https://dev.twitch.tv/console/apps"), nullptr, nullptr); })
				])
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.Visibility_Lambda([this]()
			{
				const FString Typed = ClientIdText.TrimStartAndEnd();
				return Context->HasClientId() && !Typed.IsEmpty() && Typed != Context->GetAppliedClientId()
					? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SampleUI::IndentedFill(SampleUI::Text(FText::FromString(TEXT("The Client ID changed. Press Use this Client ID to switch to it.")),
					SampleUI::LevelColor(ESampleLogLevel::Warning)))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Row(TEXT("EventSubProxy"),
				SampleUI::Muted(FText::FromString(Ctx->UsesEventSubProxy()
					? TEXT("On: events come from a local EventSubProxy (development only). Change it in Project Settings > Plugins > Twitch, then restart.")
					: TEXT("Off: events come from Twitch. To test with a local EventSubProxy, enable it in Project Settings > Plugins > Twitch, then restart."))))
		];
}

TSharedRef<SWidget> SAuthorizationPage::MakeScopesSection()
{
	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("2. Scopes"), TEXT("Permissions requested when you log in. To change them after logging in, log out and log in again."))
		];

	for (int32 Index = 0; Index < Scopes.Num(); ++Index)
	{
		Box->AddSlot().AutoHeight().Padding(0, 1)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(SampleUI::LabelWidth + 80.f)
				[
					SampleUI::CheckBox(Scopes[Index].Scope, &Scopes[Index].bEnabled)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SampleUI::Muted(FText::FromString(Scopes[Index].UsedFor))
			]
		];
	}

	Box->AddSlot().AutoHeight().Padding(0, 4)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SampleUI::Button(TEXT("Select all"), [this]() { for (FScopeOption& Option : Scopes) Option.bEnabled = true; })
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SampleUI::Button(TEXT("Select none"), [this]() { for (FScopeOption& Option : Scopes) Option.bEnabled = false; })
		]
	];
	return Box;
}

TSharedRef<SWidget> SAuthorizationPage::MakeLoginSection()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();

	TSharedRef<SWidget> CodeBox = SampleUI::Panel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Text(FText::FromString(TEXT("Open the activation page and enter this code:"))) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SNew(STextBlock).Font(SampleUI::Font(26, true)).Text_Lambda([this]() { return FText::FromString(UserCode); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Text(TAttribute<FText>::CreateLambda([this]() { return FText::FromString(TEXT("Activation page: ") + ActivationUri); }))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
			[
				SampleUI::Button(TEXT("Open activation page"), [this]() { FPlatformProcess::LaunchURL(*ActivationUri, nullptr, nullptr); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SampleUI::Button(TEXT("Copy code"), [this]()
				{
					FPlatformApplicationMisc::ClipboardCopy(*UserCode);
					Context->Info(TEXT("Code copied to the clipboard."));
				})
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("This page updates by itself once you approve the login on Twitch."))) ]);

	TSharedRef<SWidget> LoggedInBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Row(TEXT("Granted scopes"), SampleUI::Text(TAttribute<FText>::CreateLambda([this]()
			{
				const TArray<FString>& Granted = Context->GetGrantedScopes();
				return FText::FromString(Granted.Num() == 0 ? FString(TEXT("(none)")) : FString::Join(Granted, TEXT(", ")));
			})))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SampleUI::Indented(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					SampleUI::ActionButton(Ctx, TEXT("Get my user info"), [this]()
					{
						const TWeakPtr<SAuthorizationPage> WeakThis = SharedThis(this);
						Context->Call<TwitchSDK::UserInfo>(TEXT("Get my user info"),
							[](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.GetMyUserInfo(Resolve, Reject); },
							[WeakThis](const TwitchSDK::UserInfo& Info)
							{
								const TSharedPtr<SAuthorizationPage> This = WeakThis.Pin();
								if (!This || !This->UserInfoBox.IsValid())
									return;
								This->UserInfoBox->ClearChildren();
								This->UserInfoBox->AddSlot().AutoHeight()
								[
									SampleUI::Panel(
										SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Display name"), ToFString(Info.DisplayName)) ]
										+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Login"), ToFString(Info.LoginName)) ]
										+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("User ID"), ToFString(Info.ChannelId)) ]
										+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Broadcaster type"), Info.BroadcasterType.size() ? ToFString(Info.BroadcasterType) : FString(TEXT("(none)"))) ]
										+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Email"), ToFString(Info.Email)) ])
								];
								This->Context->Success(FString::Printf(TEXT("You are logged in as %s."), *ToFString(Info.DisplayName)));
							});
					})
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SampleUI::ActionButton(Ctx, TEXT("Log out"), [this]()
					{
						const TWeakPtr<SAuthorizationPage> WeakThis = SharedThis(this);
						Context->CallVoid(TEXT("Log out"),
							[](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.LogOut(Resolve, Reject); },
							[WeakThis]()
							{
								if (const TSharedPtr<SAuthorizationPage> This = WeakThis.Pin())
								{
									if (This->UserInfoBox.IsValid())
										This->UserInfoBox->ClearChildren();
									This->Context->Success(TEXT("Logged out. The saved token was removed."));
									This->Context->RefreshAuthState();
								}
							});
					})
				])
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(UserInfoBox, SVerticalBox) ];

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Section(TEXT("3. Log in")) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Row(TEXT("Status"), SampleUI::Text(TAttribute<FText>::CreateLambda([this]() { return Context->GetStatusText(); })))
		]
		// Logged out: the Log in button and, once requested, the device code.
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return Context->IsLoggedIn() ? EVisibility::Collapsed : EVisibility::Visible; })
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[
					SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Log in with Twitch"), [this]() { LogIn(); }, true, /*bRequiresLogin*/ false))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.Visibility_Lambda([this]() { return UserCode.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
					[
						CodeBox
					]
				]
			]
		]
		// Logged in: granted scopes, user info, log out.
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox)
			.Visibility_Lambda([this]() { return Context->IsLoggedIn() ? EVisibility::Visible : EVisibility::Collapsed; })
			[
				LoggedInBox
			]
		];
}

void SAuthorizationPage::LogIn()
{
	TArray<FString> Requested;
	for (const FScopeOption& Option : Scopes)
	{
		if (Option.bEnabled)
			Requested.Add(Option.Scope);
	}
	const FString ScopeList = FString::Join(Requested, TEXT(" "));

	const TWeakPtr<SAuthorizationPage> WeakThis = SharedThis(this);
	Context->Call<TwitchSDK::AuthenticationInfo>(TEXT("Log in with Twitch"),
		[ScopeList](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.GetAuthenticationInfo(FromFString(ScopeList), Resolve, Reject); },
		[WeakThis](const TwitchSDK::AuthenticationInfo& Info)
		{
			const TSharedPtr<SAuthorizationPage> This = WeakThis.Pin();
			if (!This)
				return;
			if (Info.UserCode.size() == 0)
			{
				This->Context->Info(TEXT("Already logged in."));
				return;
			}
			This->UserCode = ToFString(Info.UserCode);
			This->ActivationUri = ToFString(Info.Uri);
			This->Context->Info(FString::Printf(TEXT("Enter code %s at %s to finish logging in."), *This->UserCode, *This->ActivationUri));
			This->Context->RefreshAuthState();
			FPlatformProcess::LaunchURL(*This->ActivationUri, nullptr, nullptr);
		});
}
