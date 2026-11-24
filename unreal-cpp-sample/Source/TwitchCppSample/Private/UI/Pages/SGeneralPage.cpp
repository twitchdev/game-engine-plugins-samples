#include "UI/Pages/SGeneralPage.h"

#include "HAL/PlatformProcess.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/SampleWidgets.h"

using TwitchSDK::ToFString;
using TwitchSDK::FromFString;
using FApi = FTwitchSampleContext::FApi;

namespace
{
	constexpr int32 MaxFeedEntries = 50;
	const TCHAR* LeaderboardPeriods[] = { TEXT("day"), TEXT("week"), TEXT("month"), TEXT("year"), TEXT("all") };

	FString TierName(const FString& Tier)
	{
		if (Tier == TEXT("1000")) return TEXT("1");
		if (Tier == TEXT("2000")) return TEXT("2");
		if (Tier == TEXT("3000")) return TEXT("3");
		return SampleUI::OrDash(Tier);
	}

	bool IsLive(const TwitchSDK::StreamInfo& Stream)
	{
		return Stream.Id.size() > 0;
	}

	FString Join(const std::vector<TwitchSDK::string_holder>& Items)
	{
		TArray<FString> Parts;
		for (const TwitchSDK::string_holder& Item : Items)
			Parts.Add(ToFString(Item));
		return FString::Join(Parts, TEXT(", "));
	}

	/** A collapsible section, expanded at first. */
	TSharedRef<SWidget> Expandable(const FString& Heading, const FString& Description, const TSharedRef<SWidget>& Body)
	{
		return SNew(SBox).Padding(FMargin(0, 10, 0, 0))
			[
				SNew(SExpandableArea)
				.InitiallyCollapsed(false)
				.HeaderContent()
				[
					SNew(STextBlock).Text(FText::FromString(Heading)).Font(SampleUI::Font(14, true))
				]
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[ SampleUI::Muted(FText::FromString(Description)) ]
					+ SVerticalBox::Slot().AutoHeight()[ Body ]
				]
			];
	}

	/** A horizontal row of widgets with even spacing. */
	TSharedRef<SHorizontalBox> Buttons(std::initializer_list<TSharedRef<SWidget>> Items)
	{
		TSharedRef<SHorizontalBox> Box = SNew(SHorizontalBox);
		for (const TSharedRef<SWidget>& Item : Items)
			Box->AddSlot().AutoWidth().Padding(0, 0, 6, 0)[ Item ];
		return Box;
	}
}

void SGeneralPage::Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext)
{
	Context = InContext;

	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);
	auto Feeder = [WeakThis](const FString& Line) { if (const TSharedPtr<SGeneralPage> This = WeakThis.Pin()) This->AddToFeed(Line); };

	Follows = MakeShared<TSampleEventSubscription<TwitchSDK::ChannelFollowEvent>>(TEXT("follows"), TwitchSDK::EventStreamKind::Follower,
		&TwitchSDK::ITwitchSDKApiCommon::WaitForChannelFollowEvent,
		[Feeder](const TwitchSDK::ChannelFollowEvent& E) { Feeder(FString::Printf(TEXT("%s followed the channel."), *ToFString(E.UserDisplayName))); });

	Subscriptions = MakeShared<TSampleEventSubscription<TwitchSDK::ChannelSubscribeEvent>>(TEXT("subscriptions"), TwitchSDK::EventStreamKind::Subscription,
		&TwitchSDK::ITwitchSDKApiCommon::WaitForChannelSubscribeEvent,
		[Feeder](const TwitchSDK::ChannelSubscribeEvent& E)
		{
			const FString Tier = TierName(ToFString(E.Tier));
			Feeder(E.IsGift
				? FString::Printf(TEXT("%s received a gifted tier %s subscription."), *ToFString(E.UserDisplayName), *Tier)
				: FString::Printf(TEXT("%s subscribed at tier %s (%d months total)."), *ToFString(E.UserDisplayName), *Tier, E.CumulativeMonths));
		});

	Cheers = MakeShared<TSampleEventSubscription<TwitchSDK::ChannelCheerEvent>>(TEXT("cheers"), TwitchSDK::EventStreamKind::Cheer,
		&TwitchSDK::ITwitchSDKApiCommon::WaitForChannelCheerEvent,
		[Feeder](const TwitchSDK::ChannelCheerEvent& E)
		{
			Feeder(FString::Printf(TEXT("%s cheered %lld bits: %s"),
				E.IsAnonymous ? TEXT("An anonymous viewer") : *ToFString(E.UserDisplayName), static_cast<long long>(E.Bits), *ToFString(E.Message)));
		});

	Raids = MakeShared<TSampleEventSubscription<TwitchSDK::ChannelRaidEvent>>(TEXT("raids"), TwitchSDK::EventStreamKind::ChannelRaid,
		&TwitchSDK::ITwitchSDKApiCommon::WaitForChannelRaidEvent,
		[Feeder](const TwitchSDK::ChannelRaidEvent& E)
		{
			Feeder(FString::Printf(TEXT("%s raided %s with %lld viewers."),
				*ToFString(E.FromBroadcasterName), *ToFString(E.ToBroadcasterName), static_cast<long long>(E.Viewers)));
		});

	HypeTrains = MakeShared<TSampleEventSubscription<TwitchSDK::HypeTrainEvent>>(TEXT("Hype Train updates"), TwitchSDK::EventStreamKind::HypeTrain,
		&TwitchSDK::ITwitchSDKApiCommon::WaitForHypeTrainEvent,
		[Feeder](const TwitchSDK::HypeTrainEvent& E)
		{
			Feeder(E.EndedAt.size() == 0
				? FString::Printf(TEXT("Hype Train level %d: %lld / %lld points."), E.Level, static_cast<long long>(E.Progress), static_cast<long long>(E.Goal))
				: FString::Printf(TEXT("Hype Train ended at level %d with %lld points."), E.Level, static_cast<long long>(E.TotalPoints)));
		});

	Redemptions = MakeShared<TSampleEventSubscription<TwitchSDK::CustomRewardEvent>>(TEXT("channel point redemptions"), TwitchSDK::EventStreamKind::CustomRewardRedemption,
		&TwitchSDK::ITwitchSDKApiCommon::WaitForCustomRewardEvent,
		[WeakThis](const TwitchSDK::CustomRewardEvent& E) { if (const TSharedPtr<SGeneralPage> This = WeakThis.Pin()) This->OnRedemption(E); });

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeConnectionHint() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeUsers() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeStreams() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeSubscriptionCheck() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeChannelInfo() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeMarkersAndClips() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeBitsLeaderboard() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeRewards() ]
		+ SVerticalBox::Slot().AutoHeight()[ MakeEvents() ]
	];

	AuthChangedHandle = Context->OnAuthStateChanged.AddSP(this, &SGeneralPage::OnAuthStateChanged);
	RebuildRedemptions();
	RebuildFeed();
}

SGeneralPage::~SGeneralPage()
{
	if (Context.IsValid())
		Context->OnAuthStateChanged.Remove(AuthChangedHandle);
}

void SGeneralPage::ReleaseApiResources()
{
	StopAllEvents(/*bLog*/ false);
	PendingRedemptions.Reset();
	RebuildRedemptions();
}

void SGeneralPage::OnAuthStateChanged()
{
	// Live events need a login; stop them after a log out so the toggles match what's running.
	if (!Context->IsLoggedIn())
		StopAllEvents(/*bLog*/ true);
}

void SGeneralPage::StopAllEvents(bool bLog)
{
	Follows->Stop(bLog);
	Subscriptions->Stop(bLog);
	Cheers->Stop(bLog);
	Raids->Stop(bLog);
	HypeTrains->Stop(bLog);
	Redemptions->Stop(bLog);
}

// ---------------------------------------------------------------------------- Users

TSharedRef<SWidget> SGeneralPage::MakeUsers()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);
	auto Show = [WeakThis](const TwitchSDK::UserInfo& Info) { if (const auto This = WeakThis.Pin()) This->ShowUser(Info); };

	return Expandable(TEXT("Users"), TEXT("Look up any Twitch user by login name or user ID."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Get my user info"), [this, Show]()
			{
				Context->Call<TwitchSDK::UserInfo>(TEXT("Get my user info"),
					[](FApi& Api, auto Resolve, auto Reject) { Api.GetMyUserInfo(Resolve, Reject); }, Show);
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Login name"), &LookupLogin) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Look up by login"), [this, Show]()
				{
					const FString Login = LookupLogin.TrimStartAndEnd();
					Context->Call<TwitchSDK::UserInfo>(TEXT("Look up by login"),
						[Login](FApi& Api, auto Resolve, auto Reject) { Api.GetUserInfoByLoginName(FromFString(Login), Resolve, Reject); }, Show);
				},
				TAttribute<bool>::CreateLambda([this]() { return !LookupLogin.TrimStartAndEnd().IsEmpty(); })))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("User ID"), &LookupId) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Look up by ID"), [this, Show]()
				{
					const FString Id = LookupId.TrimStartAndEnd();
					Context->Call<TwitchSDK::UserInfo>(TEXT("Look up by ID"),
						[Id](FApi& Api, auto Resolve, auto Reject) { Api.GetUserInfoById(FromFString(Id), Resolve, Reject); }, Show);
				},
				TAttribute<bool>::CreateLambda([this]() { return !LookupId.TrimStartAndEnd().IsEmpty(); })))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(UserBox, SVerticalBox) ]);
}

void SGeneralPage::ShowUser(const TwitchSDK::UserInfo& Info)
{
	UserBox->ClearChildren();
	if (Info.ChannelId.size() == 0)
	{
		Context->Warning(TEXT("No user found."));
		return;
	}

	LastUserId = ToFString(Info.ChannelId);
	LastUserLogin = ToFString(Info.LoginName);

	UserBox->AddSlot().AutoHeight().Padding(0, 4)
	[
		SampleUI::Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Display name"), ToFString(Info.DisplayName)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Login"), LastUserLogin) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("User ID"), LastUserId) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Broadcaster type"), Info.BroadcasterType.size() ? ToFString(Info.BroadcasterType) : FString(TEXT("(none)"))) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Description"), ToFString(Info.Description)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Created"), ToFString(Info.CreatedAt)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Profile image"), ToFString(Info.ProfileImageUrl)) ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
			[
				SampleUI::Indented(Buttons({
					SampleUI::Button(TEXT("Use this ID for the subscription check"), [this]() { SubscriptionBroadcasterId = LastUserId; }),
					SampleUI::Button(TEXT("Open channel"), [this]() { FPlatformProcess::LaunchURL(*(TEXT("https://www.twitch.tv/") + LastUserLogin), nullptr, nullptr); }),
				}))
			])
	];
	Context->Success(FString::Printf(TEXT("Found %s (user ID %s)."), *ToFString(Info.DisplayName), *LastUserId));
}

// ---------------------------------------------------------------------------- Streams

TSharedRef<SWidget> SGeneralPage::MakeStreams()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);

	return Expandable(TEXT("Streams"), TEXT("See your own stream and find out which channels are live."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Get my stream info"), [this, WeakThis]()
			{
				Context->Call<TwitchSDK::StreamInfo>(TEXT("Get my stream info"),
					[](FApi& Api, auto Resolve, auto Reject) { Api.GetMyStreamInfo(Resolve, Reject); },
					[WeakThis](const TwitchSDK::StreamInfo& Stream)
					{
						if (const TSharedPtr<SGeneralPage> This = WeakThis.Pin())
						{
							This->ShowStream(This->MyStreamBox, Stream);
							This->Context->Success(IsLive(Stream) ? TEXT("You are live: ") + ToFString(Stream.Title) : FString(TEXT("Your channel is offline.")));
						}
					});
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(MyStreamBox, SVerticalBox) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Channel logins"), &StreamLogins, TEXT("Separate several logins with commas (up to 100).")) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Find live streams"), [this, WeakThis]()
				{
					const TArray<FString> Logins = SampleUI::SplitList(StreamLogins);
					TwitchSDK::StreamQuery Query;
					Query.UserLogins = ToOwned(Logins);
					Query.First = FMath::Min(Logins.Num(), 100);
					const int32 Asked = Logins.Num();
					Context->Call<TwitchSDK::StreamQueryResult>(TEXT("Find live streams"),
						[Query](FApi& Api, auto Resolve, auto Reject) { Api.QueryStreams(Query, Resolve, Reject); },
						[WeakThis, Asked](const TwitchSDK::StreamQueryResult& Result)
						{
							const TSharedPtr<SGeneralPage> This = WeakThis.Pin();
							if (!This)
								return;
							This->FoundStreamsBox->ClearChildren();
							if (Result.Streams.empty())
								This->FoundStreamsBox->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("None of those channels are live."))) ];
							for (const TwitchSDK::StreamInfo& Stream : Result.Streams)
							{
								TSharedPtr<SVerticalBox> One;
								This->FoundStreamsBox->AddSlot().AutoHeight()[ SAssignNew(One, SVerticalBox) ];
								This->ShowStream(One, Stream);
							}
							This->Context->Success(FString::Printf(TEXT("%d of %d channel(s) are live."), static_cast<int32>(Result.Streams.size()), Asked));
						});
				},
				TAttribute<bool>::CreateLambda([this]() { return SampleUI::SplitList(StreamLogins).Num() > 0; })))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(FoundStreamsBox, SVerticalBox) ]);
}

void SGeneralPage::ShowStream(const TSharedPtr<SVerticalBox>& Target, const TwitchSDK::StreamInfo& Stream)
{
	Target->ClearChildren();
	if (!IsLive(Stream))
	{
		Target->AddSlot().AutoHeight()[ SampleUI::Panel(SampleUI::Muted(FText::FromString(TEXT("Offline.")))) ];
		return;
	}
	Target->AddSlot().AutoHeight().Padding(0, 3)
	[
		SampleUI::Panel(
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Channel"), ToFString(Stream.UserName)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Title"), ToFString(Stream.Title)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Category"), ToFString(Stream.GameName)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Viewers"), FString::Printf(TEXT("%lld"), static_cast<long long>(Stream.ViewerCount))) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Started"), ToFString(Stream.StartedAt)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Language"), ToFString(Stream.Language)) ]
			+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Tags"), Join(Stream.Tags)) ])
	];
}

// ---------------------------------------------------------------------------- Subscription check

TSharedRef<SWidget> SGeneralPage::MakeSubscriptionCheck()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);

	return Expandable(TEXT("Subscription check"),
		TEXT("Checks whether the logged-in user subscribes to a channel. Find a channel's user ID with the Users section above."),
		SNew(SVerticalBox)
		// Bound with a lambda (not SampleUI::TextRow) so "Use this ID" in the Users section can fill it in.
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Row(TEXT("Broadcaster user ID"),
				SNew(SEditableTextBox)
				.Font(SampleUI::Font(10))
				.Text_Lambda([this]() { return FText::FromString(SubscriptionBroadcasterId); })
				.OnTextChanged_Lambda([this](const FText& Text) { SubscriptionBroadcasterId = Text.ToString(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Check my subscription"), [this, WeakThis]()
				{
					const FString Id = SubscriptionBroadcasterId.TrimStartAndEnd();
					Context->Call<TwitchSDK::UserSubscriptionCheckResult>(TEXT("Check my subscription"),
						[Id](FApi& Api, auto Resolve, auto Reject) { Api.CheckUserSubscription(FromFString(Id), Resolve, Reject); },
						[WeakThis, Id](const TwitchSDK::UserSubscriptionCheckResult& Result)
						{
							const TSharedPtr<SGeneralPage> This = WeakThis.Pin();
							if (!This)
								return;
							const FString Tier = TierName(ToFString(Result.Tier));
							TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Broadcaster ID"), Id) ]
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Subscribed"), Result.IsSubscribed ? TEXT("Yes") : TEXT("No")) ];
							if (Result.IsSubscribed)
							{
								Box->AddSlot().AutoHeight()[ SampleUI::KeyValue(TEXT("Tier"), Tier) ];
								Box->AddSlot().AutoHeight()[ SampleUI::KeyValue(TEXT("Gift"), Result.IsGift ? TEXT("Yes, from ") + ToFString(Result.GifterName) : FString(TEXT("No"))) ];
							}
							This->SubscriptionBox->ClearChildren();
							This->SubscriptionBox->AddSlot().AutoHeight().Padding(0, 4)[ SampleUI::Panel(Box) ];
							This->Context->Success(Result.IsSubscribed
								? FString::Printf(TEXT("You subscribe to %s at tier %s."), *Id, *Tier)
								: FString::Printf(TEXT("You don't subscribe to %s."), *Id));
						});
				},
				TAttribute<bool>::CreateLambda([this]() { return !SubscriptionBroadcasterId.TrimStartAndEnd().IsEmpty(); })))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(SubscriptionBox, SVerticalBox) ]);
}

// ---------------------------------------------------------------------------- Channel information

TSharedRef<SWidget> SGeneralPage::MakeChannelInfo()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	return Expandable(TEXT("Channel information"), TEXT("Updates your channel. Fill in only the fields you want to change."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Stream title"), &ChannelTitle) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Category (game) ID"), &ChannelGameId, TEXT("A numeric category ID, e.g. 509658 for Just Chatting.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Language"), &ChannelLanguage, TEXT("A two-letter code such as en or de.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Tags"), &ChannelTags, TEXT("Separate tags with commas. Tags can't contain spaces.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Indented(SampleUI::CheckBox(TEXT("Remove all tags"), &bClearTags)) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Update channel"), [this]() { UpdateChannel(); }))
		]);
}

void SGeneralPage::UpdateChannel()
{
	const FString Title = ChannelTitle.TrimStartAndEnd();
	const FString GameId = ChannelGameId.TrimStartAndEnd();
	const FString Language = ChannelLanguage.TrimStartAndEnd();
	const TArray<FString> Tags = SampleUI::SplitList(ChannelTags);
	const bool bSendTags = bClearTags || Tags.Num() > 0;

	if (Title.IsEmpty() && GameId.IsEmpty() && Language.IsEmpty() && !bSendTags)
	{
		Context->Error(TEXT("Fill in at least one field to change."));
		return;
	}

	TwitchSDK::ModifyChannelInfoRequest Request;
	Request.Title = ToOwned(Title);
	Request.GameId = ToOwned(GameId);
	Request.Language = ToOwned(Language);
	// The core sends "delay" whenever it's 0 or more, and the struct defaults to 0, which would
	// reset the stream delay (and fails for non-Partners). -1 leaves it unchanged.
	Request.Delay = -1;
	if (bSendTags)
	{
		if (!bClearTags)
			Request.Tags = ToOwned(Tags);
		Request.ForceUpdateTags = true; // with an empty list this removes all tags
	}

	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);
	Context->CallVoid(TEXT("Update channel"),
		[Request](FApi& Api, auto Resolve, auto Reject) { Api.ModifyChannelInformation(Request, Resolve, Reject); },
		[WeakThis]() { if (const auto This = WeakThis.Pin()) This->Context->Success(TEXT("Channel information updated.")); });
}

// ---------------------------------------------------------------------------- Markers and clips

TSharedRef<SWidget> SGeneralPage::MakeMarkersAndClips()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);

	return Expandable(TEXT("Stream markers and clips"), TEXT("Both need your channel to be live."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Marker description"), &MarkerDescription, TEXT("Up to 140 characters.")) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Create stream marker"), [this, WeakThis]()
			{
				const FString Description = MarkerDescription.TrimStartAndEnd();
				Context->Call<TwitchSDK::StreamMarkerInfo>(TEXT("Create stream marker"),
					[Description](FApi& Api, auto Resolve, auto Reject) { Api.CreateStreamMarker(FromFString(Description), Resolve, Reject); },
					[WeakThis](const TwitchSDK::StreamMarkerInfo& Marker)
					{
						const TSharedPtr<SGeneralPage> This = WeakThis.Pin();
						if (!This)
							return;
						This->MarkerBox->ClearChildren();
						This->MarkerBox->AddSlot().AutoHeight().Padding(0, 4)
						[
							SampleUI::Panel(
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Marker ID"), ToFString(Marker.Id)) ]
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Description"), ToFString(Marker.Description)) ]
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Position"), FString::Printf(TEXT("%lld seconds"), static_cast<long long>(Marker.PositionSeconds))) ]
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Created"), ToFString(Marker.CreatedAt)) ])
						];
						This->Context->Success(FString::Printf(TEXT("Marker created %lld seconds into the stream."), static_cast<long long>(Marker.PositionSeconds)));
					});
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(MarkerBox, SVerticalBox) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[ SampleUI::Indented(SampleUI::CheckBox(TEXT("Account for the stream delay when clipping"), &bClipHasDelay)) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Create clip"), [this, WeakThis]()
			{
				const bool bDelay = bClipHasDelay;
				Context->Info(TEXT("Creating a clip. Twitch takes a few seconds to process it ..."));
				Context->Call<TwitchSDK::ClipInfo>(TEXT("Create clip"),
					[bDelay](FApi& Api, auto Resolve, auto Reject) { Api.CreateClip(bDelay, Resolve, Reject); },
					[WeakThis](const TwitchSDK::ClipInfo& Clip)
					{
						const TSharedPtr<SGeneralPage> This = WeakThis.Pin();
						if (!This)
							return;
						const FString Url = ToFString(Clip.Url);
						This->ClipBox->ClearChildren();
						This->ClipBox->AddSlot().AutoHeight().Padding(0, 4)
						[
							SampleUI::Panel(
								SNew(SVerticalBox)
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Clip ID"), ToFString(Clip.Id)) ]
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("URL"), Url) ]
								+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Duration"), FString::Printf(TEXT("%d seconds"), Clip.Duration)) ]
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
								[
									SampleUI::Indented(SampleUI::Button(TEXT("Open clip"), [Url]() { FPlatformProcess::LaunchURL(*Url, nullptr, nullptr); }, !Url.IsEmpty()))
								])
						];
						This->Context->Success(TEXT("Clip created: ") + Url);
					});
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(ClipBox, SVerticalBox) ]);
}

// ---------------------------------------------------------------------------- Bits leaderboard

TSharedRef<SWidget> SGeneralPage::MakeBitsLeaderboard()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);

	TSharedRef<SHorizontalBox> Periods = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(LeaderboardPeriods)); ++Index)
	{
		Periods->AddSlot().AutoWidth().Padding(0, 0, 12, 0)
		[
			SNew(SCheckBox)
			.Style(FCoreStyle::Get(), "RadioButton")
			.IsChecked_Lambda([this, Index]() { return LeaderboardPeriod == Index ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, Index](ECheckBoxState) { LeaderboardPeriod = Index; })
			[
				SNew(STextBlock).Text(FText::FromString(LeaderboardPeriods[Index])).Font(SampleUI::Font(10))
			]
		];
	}

	return Expandable(TEXT("Bits leaderboard"), TEXT("The top cheerers on your channel."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Entries"), &LeaderboardCount, TEXT("1 to 100.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Row(TEXT("Period"), Periods) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Get Bits leaderboard"), [this, WeakThis]()
			{
				int32 Count = 0;
				if (!SampleUI::TryParseInt(*Context, LeaderboardCount, TEXT("Entries"), 1, 100, Count))
					return;
				TwitchSDK::BitsLeaderboardRequest Request;
				Request.Count = Count;
				Request.Period = ToOwned(LeaderboardPeriods[LeaderboardPeriod]);
				Context->Call<TwitchSDK::BitsLeaderboard>(TEXT("Get Bits leaderboard"),
					[Request](FApi& Api, auto Resolve, auto Reject) { Api.GetBitsLeaderboard(Request, Resolve, Reject); },
					[WeakThis](const TwitchSDK::BitsLeaderboard& Board)
					{
						const TSharedPtr<SGeneralPage> This = WeakThis.Pin();
						if (!This)
							return;
						TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
						if (Board.Data.empty())
							Box->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("Nobody has cheered in this period."))) ];
						for (const TwitchSDK::BitsLeaderboardEntry& Entry : Board.Data)
						{
							Box->AddSlot().AutoHeight()
							[
								SampleUI::KeyValue(FString::Printf(TEXT("#%lld  %s"), static_cast<long long>(Entry.Rank), *ToFString(Entry.UserName)),
									FString::Printf(TEXT("%lld bits"), static_cast<long long>(Entry.Score)))
							];
						}
						This->LeaderboardBox->ClearChildren();
						This->LeaderboardBox->AddSlot().AutoHeight().Padding(0, 4)[ SampleUI::Panel(Box) ];
						This->Context->Success(FString::Printf(TEXT("Loaded %d leaderboard entries."), static_cast<int32>(Board.Data.size())));
					});
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(LeaderboardBox, SVerticalBox) ]);
}

// ---------------------------------------------------------------------------- Channel point rewards

TSharedRef<SWidget> SGeneralPage::MakeRewards()
{
	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);

	return Expandable(TEXT("Channel point rewards"),
		TEXT("Creates a custom reward on your channel. Replacing rewards affects only rewards made with this Client ID. "
			 "Turn on channel point redemptions under Live events to see and resolve redemptions."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Reward title"), &RewardTitle, TEXT("Up to 45 characters, unique on your channel.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Cost (channel points)"), &RewardCost) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Prompt"), &RewardPrompt) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Indented(SampleUI::CheckBox(TEXT("Viewers must enter text when redeeming"), &bRewardNeedsInput)) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
		[
			SampleUI::Indented(Buttons({
				SampleUI::ActionButton(Ctx, TEXT("Create reward"), [this, WeakThis]()
				{
					int32 Cost = 0;
					if (!SampleUI::ValidateText(*Context, RewardTitle, TEXT("Reward title"), 45) ||
						!SampleUI::TryParseInt(*Context, RewardCost, TEXT("Cost"), 1, MAX_int32, Cost))
						return;

					TwitchSDK::CustomRewardDefinition Reward;
					Reward.Title = ToOwned(RewardTitle.TrimStartAndEnd());
					Reward.Cost = Cost;
					Reward.Prompt = ToOwned(RewardPrompt.TrimStartAndEnd());
					Reward.IsEnabled = true;
					Reward.IsUserInputRequired = bRewardNeedsInput;

					TwitchSDK::CustomRewardList List;
					List.Rewards.push_back(Reward);
					const FString Title = RewardTitle.TrimStartAndEnd();
					Context->CallVoid(TEXT("Create reward"),
						[List](FApi& Api, auto Resolve, auto Reject) { Api.ReplaceCustomRewards(List, Resolve, Reject); },
						[WeakThis, Title, Cost]()
						{
							if (const auto This = WeakThis.Pin())
								This->Context->Success(FString::Printf(TEXT("Reward \"%s\" is now on your channel for %d points."), *Title, Cost));
						});
				}),
				SampleUI::ActionButton(Ctx, TEXT("Remove sample rewards"), [this, WeakThis]()
				{
					Context->CallVoid(TEXT("Remove sample rewards"),
						[](FApi& Api, auto Resolve, auto Reject) { Api.ReplaceCustomRewards(TwitchSDK::CustomRewardList(), Resolve, Reject); },
						[WeakThis]() { if (const auto This = WeakThis.Pin()) This->Context->Success(TEXT("Removed the rewards created with this Client ID.")); });
				}),
			}))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(RedemptionsBox, SVerticalBox) ]);
}

void SGeneralPage::OnRedemption(const TwitchSDK::CustomRewardEvent& Event)
{
	const FString Input = ToFString(Event.UserInput);
	AddToFeed(FString::Printf(TEXT("%s redeemed \"%s\" for %lld points.%s"),
		*ToFString(Event.RedeemerName), *ToFString(Event.CustomRewardTitle), static_cast<long long>(Event.CustomRewardCost),
		Input.IsEmpty() ? TEXT("") : *(TEXT(" They wrote: ") + Input)));

	// Only redemptions still in the queue can be fulfilled or refunded.
	if (Event.Status != TwitchSDK::CustomRewardRedemptionState::Unfulfilled)
		return;
	const FString Id = ToFString(Event.RedemptionId);
	for (const TwitchSDK::CustomRewardEvent& Pending : PendingRedemptions)
	{
		if (ToFString(Pending.RedemptionId) == Id)
			return;
	}
	PendingRedemptions.Add(Event);
	RebuildRedemptions();
}

void SGeneralPage::ResolveRedemption(const TwitchSDK::CustomRewardEvent& Redemption, TwitchSDK::CustomRewardRedemptionState State)
{
	TwitchSDK::CustomRewardResolveRequest Request;
	Request.BroadcasterId = ToOwned(ToFString(Redemption.BroadcasterId));
	Request.CustomRewardId = ToOwned(ToFString(Redemption.CustomRewardId));
	Request.RedemptionId = ToOwned(ToFString(Redemption.RedemptionId));
	Request.Resolution = State;

	const bool bFulfill = State == TwitchSDK::CustomRewardRedemptionState::Fulfilled;
	const FString Id = ToFString(Redemption.RedemptionId);
	const FString Action = (bFulfill ? TEXT("Fulfill ") : TEXT("Refund ")) + Id;
	const FString Message = FString::Printf(TEXT("%s %s's \"%s\"."), bFulfill ? TEXT("Fulfilled") : TEXT("Refunded"),
		*ToFString(Redemption.RedeemerName), *ToFString(Redemption.CustomRewardTitle));

	const TWeakPtr<SGeneralPage> WeakThis = SharedThis(this);
	Context->CallVoid(Action,
		[Request](FApi& Api, auto Resolve, auto Reject) { Api.ResolveCustomReward(Request, Resolve, Reject); },
		[WeakThis, Id, Message]()
		{
			const TSharedPtr<SGeneralPage> This = WeakThis.Pin();
			if (!This)
				return;
			This->PendingRedemptions.RemoveAll([&Id](const TwitchSDK::CustomRewardEvent& E) { return ToFString(E.RedemptionId) == Id; });
			This->RebuildRedemptions();
			This->Context->Success(Message);
		});
}

void SGeneralPage::RebuildRedemptions()
{
	if (!RedemptionsBox.IsValid())
		return;
	RedemptionsBox->ClearChildren();
	if (PendingRedemptions.Num() == 0)
		return;

	RedemptionsBox->AddSlot().AutoHeight().Padding(0, 6, 0, 2)
	[
		SNew(STextBlock).Text(FText::FromString(TEXT("Waiting redemptions"))).Font(SampleUI::Font(12, true))
	];

	const TSharedRef<FTwitchSampleContext> Ctx = Context.ToSharedRef();
	for (const TwitchSDK::CustomRewardEvent& Redemption : PendingRedemptions)
	{
		const FString Id = ToFString(Redemption.RedemptionId);
		const FString Input = ToFString(Redemption.UserInput);
		const TwitchSDK::CustomRewardEvent Copy = Redemption;

		RedemptionsBox->AddSlot().AutoHeight().Padding(0, 3)
		[
			SampleUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Reward"), ToFString(Redemption.CustomRewardTitle)) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Viewer"), ToFString(Redemption.RedeemerName)) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Their text"), Input) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Redeemed"), ToFString(Redemption.RedeemedAt)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
				[
					SampleUI::Indented(Buttons({
						SampleUI::ActionButton(Ctx, TEXT("Fulfill ") + Id, [this, Copy]() { ResolveRedemption(Copy, TwitchSDK::CustomRewardRedemptionState::Fulfilled); }),
						SampleUI::ActionButton(Ctx, TEXT("Refund ") + Id, [this, Copy]() { ResolveRedemption(Copy, TwitchSDK::CustomRewardRedemptionState::Canceled); }),
						SampleUI::Button(TEXT("Dismiss"), [this, Id]()
						{
							PendingRedemptions.RemoveAll([&Id](const TwitchSDK::CustomRewardEvent& E) { return ToFString(E.RedemptionId) == Id; });
							RebuildRedemptions();
						}),
					}))
				])
		];
	}
}

// ---------------------------------------------------------------------------- Live events

template <class TEvent>
TSharedRef<SWidget> SGeneralPage::MakeEventToggle(const TSharedPtr<TSampleEventSubscription<TEvent>>& Subscription, const FString& Label)
{
	using EState = typename TSampleEventSubscription<TEvent>::EState;
	const TWeakPtr<TSampleEventSubscription<TEvent>> WeakSub = Subscription;

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SBox).WidthOverride(SampleUI::LabelWidth + 300.f)
			[
				SNew(SCheckBox)
				.IsEnabled_Lambda([this]() { return Context->IsLoggedIn(); })
				.IsChecked_Lambda([WeakSub]()
				{
					const auto Sub = WeakSub.Pin();
					return Sub && Sub->IsOn() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, WeakSub](ECheckBoxState State)
				{
					if (const auto Sub = WeakSub.Pin())
					{
						if (State == ECheckBoxState::Checked)
							Sub->Start(Context.ToSharedRef());
						else
							Sub->Stop();
					}
				})
				[
					SNew(STextBlock).Text(FText::FromString(Label)).Font(SampleUI::Font(10))
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Font(SampleUI::Font(10))
			.Text_Lambda([WeakSub]()
			{
				const auto Sub = WeakSub.Pin();
				if (!Sub) return FText::GetEmpty();
				switch (Sub->GetState())
				{
				case EState::Starting: return FText::FromString(TEXT("connecting ..."));
				case EState::Listening: return FText::FromString(TEXT("listening"));
				default: return FText::GetEmpty();
				}
			})
			.ColorAndOpacity_Lambda([WeakSub]()
			{
				const auto Sub = WeakSub.Pin();
				return FSlateColor(Sub && Sub->GetState() == EState::Listening ? SampleUI::LevelColor(ESampleLogLevel::Success) : SampleUI::MutedColor());
			})
		];
}

TSharedRef<SWidget> SGeneralPage::MakeEvents()
{
	return Expandable(TEXT("Live events (EventSub)"),
		TEXT("Turn on the events you want to see. To test without real viewers, enable the EventSubProxy in Project Settings > "
			 "Plugins > Twitch, restart, and send test events with the Twitch CLI."),
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)[ MakeEventToggle(Follows, TEXT("Follows (moderator:read:followers)")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)[ MakeEventToggle(Subscriptions, TEXT("Subscriptions (channel:read:subscriptions)")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)[ MakeEventToggle(Cheers, TEXT("Cheers (bits:read)")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)[ MakeEventToggle(Raids, TEXT("Raids (no scope needed)")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)[ MakeEventToggle(HypeTrains, TEXT("Hype Trains (channel:read:hype_train)")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 1)[ MakeEventToggle(Redemptions, TEXT("Channel point redemptions (channel:manage:redemptions)")) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Event feed (newest first)"))).Font(SampleUI::Font(12, true))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SampleUI::Button(TEXT("Clear feed"), [this]() { Feed.Reset(); RebuildFeed(); },
					TAttribute<bool>::CreateLambda([this]() { return Feed.Num() > 0; }))
			]
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Panel(SAssignNew(FeedBox, SVerticalBox)) ]);
}

void SGeneralPage::AddToFeed(const FString& Line)
{
	Feed.Insert(FString::Printf(TEXT("%s   %s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), *Line), 0);
	if (Feed.Num() > MaxFeedEntries)
		Feed.SetNum(MaxFeedEntries);
	Context->Info(TEXT("Event: ") + Line);
	RebuildFeed();
}

void SGeneralPage::RebuildFeed()
{
	if (!FeedBox.IsValid())
		return;
	FeedBox->ClearChildren();
	if (Feed.Num() == 0)
	{
		FeedBox->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("No events yet."))) ];
		return;
	}
	for (const FString& Line : Feed)
		FeedBox->AddSlot().AutoHeight()[ SampleUI::Text(FText::FromString(Line)) ];
}
