#pragma once

#include "CoreMinimal.h"

#include "UI/SampleEventSubscription.h"
#include "UI/SSamplePage.h"

/**
 * Everything that doesn't belong to another page: users, streams, subscriptions, channel edits,
 * stream markers, clips, the Bits leaderboard, channel point rewards, and live events.
 */
class SGeneralPage : public SSamplePage
{
public:
	SLATE_BEGIN_ARGS(SGeneralPage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext);
	virtual ~SGeneralPage() override;

	virtual FString GetTitle() const override { return TEXT("General"); }
	virtual void ReleaseApiResources() override;

private:
	TSharedRef<SWidget> MakeUsers();
	TSharedRef<SWidget> MakeStreams();
	TSharedRef<SWidget> MakeSubscriptionCheck();
	TSharedRef<SWidget> MakeChannelInfo();
	TSharedRef<SWidget> MakeMarkersAndClips();
	TSharedRef<SWidget> MakeBitsLeaderboard();
	TSharedRef<SWidget> MakeRewards();
	TSharedRef<SWidget> MakeEvents();

	template <class TEvent>
	TSharedRef<SWidget> MakeEventToggle(const TSharedPtr<TSampleEventSubscription<TEvent>>& Subscription, const FString& Label);

	void ShowUser(const TwitchSDK::UserInfo& Info);
	void ShowStream(const TSharedPtr<SVerticalBox>& Target, const TwitchSDK::StreamInfo& Stream);
	void UpdateChannel();
	void OnRedemption(const TwitchSDK::CustomRewardEvent& Event);
	void ResolveRedemption(const TwitchSDK::CustomRewardEvent& Redemption, TwitchSDK::CustomRewardRedemptionState State);
	void RebuildRedemptions();
	void AddToFeed(const FString& Line);
	void RebuildFeed();
	void StopAllEvents(bool bLog);
	void OnAuthStateChanged();

	// Users
	FString LookupLogin;
	FString LookupId;
	FString LastUserId;
	FString LastUserLogin;
	TSharedPtr<SVerticalBox> UserBox;

	// Streams
	FString StreamLogins;
	TSharedPtr<SVerticalBox> MyStreamBox;
	TSharedPtr<SVerticalBox> FoundStreamsBox;

	// Subscription check
	FString SubscriptionBroadcasterId;
	TSharedPtr<SVerticalBox> SubscriptionBox;

	// Channel information
	FString ChannelTitle;
	FString ChannelGameId;
	FString ChannelLanguage;
	FString ChannelTags;
	bool bClearTags = false;

	// Markers and clips
	FString MarkerDescription = TEXT("Sample marker");
	TSharedPtr<SVerticalBox> MarkerBox;
	bool bClipHasDelay = false;
	TSharedPtr<SVerticalBox> ClipBox;

	// Bits leaderboard
	FString LeaderboardCount = TEXT("10");
	int32 LeaderboardPeriod = 4;
	TSharedPtr<SVerticalBox> LeaderboardBox;

	// Channel point rewards
	FString RewardTitle = TEXT("Sample reward");
	FString RewardCost = TEXT("100");
	FString RewardPrompt = TEXT("Redeemed from the Twitch SDK Unreal C++ sample");
	bool bRewardNeedsInput = false;
	TArray<TwitchSDK::CustomRewardEvent> PendingRedemptions;
	TSharedPtr<SVerticalBox> RedemptionsBox;

	// Live events
	TSharedPtr<TSampleEventSubscription<TwitchSDK::ChannelFollowEvent>> Follows;
	TSharedPtr<TSampleEventSubscription<TwitchSDK::ChannelSubscribeEvent>> Subscriptions;
	TSharedPtr<TSampleEventSubscription<TwitchSDK::ChannelCheerEvent>> Cheers;
	TSharedPtr<TSampleEventSubscription<TwitchSDK::ChannelRaidEvent>> Raids;
	TSharedPtr<TSampleEventSubscription<TwitchSDK::HypeTrainEvent>> HypeTrains;
	TSharedPtr<TSampleEventSubscription<TwitchSDK::CustomRewardEvent>> Redemptions;
	TArray<FString> Feed;
	TSharedPtr<SVerticalBox> FeedBox;

	FDelegateHandle AuthChangedHandle;
};
