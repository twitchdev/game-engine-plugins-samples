
#pragma once

#include "CoreMinimal.h"
#include <vector>
#include "TwitchSDK.h"
#include "TwitchSDKStructs.generated.h"


UENUM(BlueprintType)
enum class FTwitchSDKLogLevel : uint8
{
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
};

/** The current user authentication status. */
UENUM(BlueprintType)
enum class FTwitchSDKAuthStatus : uint8
{
    /** The core library is logged out. */
    LoggedOut = 0,
    /** The status can not be determined because an operation affecting it is currently in progress. */
    Loading = 1,
    /** The authorization code flow is in progress and we are currently waiting for the user to authorize. */
    WaitingForCode = 2,
    /** We have valid OAuth credentials. */
    LoggedIn = 3,
};

UENUM(BlueprintType)
enum class FTwitchSDKHypeTrainContributionType : uint8
{
    /** Bits, from cheering, Power-ups, and Extensions. */
    Bits = 0,
    /** Subscriptions, including gifted subscriptions. */
    Subscription = 1,
    /** Any other contribution method. */
    Other = 2,
};

UENUM(BlueprintType)
enum class FTwitchSDKCustomRewardRedemptionState : uint8
{
    /** There was an error determining the state of the redemption. */
    Unknown = 0,
    /** The channel points reward has been redeemed and is now waiting to be fulfilled. */
    Unfulfilled = 1,
    /** The channel points reward has been fulfilled. */
    Fulfilled = 2,
    /** The channel points reward has been canceled, and the channel points have been refunded. */
    Canceled = 3,
};

/** The fulfillment status of a Drop entitlement. */
UENUM(BlueprintType)
enum class FTwitchSDKDropFulfillmentStatus : uint8
{
    /** The entitlement's status could not be determined (e.g. it was absent or unrecognized in the API response). */
    Unknown = 0,
    /** The viewer has earned the Drop and claimed it on Twitch, but your game has not yet granted the reward. */
    Claimed = 1,
    /** Your game has granted the reward and marked the entitlement fulfilled via FulfillDropEntitlements. */
    Fulfilled = 2,
};

/** The type of an EventSub subscription. */
UENUM(BlueprintType)
enum class FTwitchSDKEventStreamKind : uint8
{
    /** A notification when a specified channel receives a subscriber. This does not include resubscribes. */
    Subscription = 0,
    /** A specified channel receives a follow. */
    Follower = 1,
    /** A user cheers on the specified channel. */
    Cheer = 2,
    /** A viewer has redeemed a custom channel points reward on the specified channel or the redemption has been updated (i.e., fulfilled or cancelled). */
    CustomRewardRedemption = 3,
    /** A Hype Train makes progress on the user's channel. Requires the channel:read:hype_train scope. */
    HypeTrain = 4,
    /** A broadcaster raids another broadcaster's channel. */
    ChannelRaid = 5,
};

/** Status of a Prediction. */
UENUM(BlueprintType)
enum class FTwitchSDKPredictionStatus : uint8
{
    /** The Prediction is active and viewers can make predictions. */
    Active = 0,
    /** The Prediction has been locked and viewers can no longer make predictions. */
    Locked = 1,
    /** A winning outcome has been chosen and the Channel Points have been distributed to the users who guessed the correct outcome. */
    Resolved = 2,
    /** The Prediction has been canceled and the Channel Points have been refunded to participants. */
    Canceled = 3,
};

/** Status of a poll. */
UENUM(BlueprintType)
enum class FTwitchSDKPollStatus : uint8
{
    /** Poll is currently in progress. */
    Active = 0,
    /** Poll has reached its PollInfo::EndedAt time. */
    Completed = 1,
    /** Poll has been manually terminated before its PollInfo::EndedAt time. */
    Terminated = 2,
    /** Poll is no longer visible on the channel. */
    Archived = 3,
    /** Poll is no longer visible to any user on Twitch. */
    Moderated = 4,
    /** Something went wrong determining the state. */
    Invalid = 5,
};

UENUM(BlueprintType)
enum class FTwitchSDKHttpMethod : uint8
{
    Get = 0,
    Post = 1,
    Put = 2,
    Patch = 3,
    Delete = 4,
};

/** The library's user authentication state. */
USTRUCT(BlueprintType)
struct FTwitchSDKAuthState {
    GENERATED_BODY()

public:
    FTwitchSDKAuthState() = default;
    FTwitchSDKAuthState(const TwitchSDK::AuthState& x) : Status(FTwitchSDKAuthStatus(x.Status)), Scopes(TwitchSDK::ToFStringArray(x.Scopes)) {}
    operator TwitchSDK::AuthState () const {
        TwitchSDK::AuthState v;
        v.Status = TwitchSDK::AuthStatus(Status);
        v.Scopes = TwitchSDK::FromFStringArray(Scopes);
        return v;
    }
    /** The current authentication status. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKAuthStatus Status = FTwitchSDKAuthStatus(0);
    /** If Status is AuthStatus.LoggedIn, this is the set of available OAuth scopes. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> Scopes;
};

/** A broadcaster raids another broadcaster's channel. */
USTRUCT(BlueprintType)
struct FTwitchSDKChannelRaidEvent {
    GENERATED_BODY()

public:
    FTwitchSDKChannelRaidEvent() = default;
    FTwitchSDKChannelRaidEvent(const TwitchSDK::ChannelRaidEvent& x) : FromBroadcasterId(TwitchSDK::ToFString(x.FromBroadcasterId)), FromBroadcasterName(TwitchSDK::ToFString(x.FromBroadcasterName)), ToBroadcasterId(TwitchSDK::ToFString(x.ToBroadcasterId)), ToBroadcasterName(TwitchSDK::ToFString(x.ToBroadcasterName)), Viewers(x.Viewers) {}
    operator TwitchSDK::ChannelRaidEvent () const {
        TwitchSDK::ChannelRaidEvent v;
        v.FromBroadcasterId = TwitchSDK::FromFString(FromBroadcasterId);
        v.FromBroadcasterName = TwitchSDK::FromFString(FromBroadcasterName);
        v.ToBroadcasterId = TwitchSDK::FromFString(ToBroadcasterId);
        v.ToBroadcasterName = TwitchSDK::FromFString(ToBroadcasterName);
        v.Viewers = Viewers;
        return v;
    }
    /** The broadcaster ID that created the raid. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString FromBroadcasterId;
    /** The broadcaster display name that created the raid. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString FromBroadcasterName;
    /** The broadcaster ID that received the raid. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ToBroadcasterId;
    /** The broadcaster display name that received the raid. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ToBroadcasterName;
    /** The number of viewers in the raid. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Viewers = 0;
};

USTRUCT(BlueprintType)
struct FTwitchSDKCustomRewardResolveRequest {
    GENERATED_BODY()

public:
    FTwitchSDKCustomRewardResolveRequest() = default;
    FTwitchSDKCustomRewardResolveRequest(const TwitchSDK::CustomRewardResolveRequest& x) : RedemptionId(TwitchSDK::ToFString(x.RedemptionId)), CustomRewardId(TwitchSDK::ToFString(x.CustomRewardId)), BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), Resolution(FTwitchSDKCustomRewardRedemptionState(x.Resolution)) {}
    operator TwitchSDK::CustomRewardResolveRequest () const {
        TwitchSDK::CustomRewardResolveRequest v;
        v.RedemptionId = TwitchSDK::FromFString(RedemptionId);
        v.CustomRewardId = TwitchSDK::FromFString(CustomRewardId);
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.Resolution = TwitchSDK::CustomRewardRedemptionState(Resolution);
        return v;
    }
    /** ID of the Custom Reward Redemption to update. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString RedemptionId;
    /** ID of the Custom Reward the redemptions to be updated are for. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CustomRewardId;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** The new status to set redemptions to. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKCustomRewardRedemptionState Resolution = FTwitchSDKCustomRewardRedemptionState(0);
};

/** Describes a contribution to a Hype Train. */
USTRUCT(BlueprintType)
struct FTwitchSDKHypeTrainContribution {
    GENERATED_BODY()

public:
    FTwitchSDKHypeTrainContribution() = default;
    FTwitchSDKHypeTrainContribution(const TwitchSDK::HypeTrainContribution& x) : UserId(TwitchSDK::ToFString(x.UserId)), UserName(TwitchSDK::ToFString(x.UserName)), Type(FTwitchSDKHypeTrainContributionType(x.Type)), Total(x.Total) {}
    operator TwitchSDK::HypeTrainContribution () const {
        TwitchSDK::HypeTrainContribution v;
        v.UserId = TwitchSDK::FromFString(UserId);
        v.UserName = TwitchSDK::FromFString(UserName);
        v.Type = TwitchSDK::HypeTrainContributionType(Type);
        v.Total = Total;
        return v;
    }
    /** The ID of the user who contributed to the Hype Train. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** The display name of the user who contributed to the Hype Train. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserName;
    /** Type of contribution. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKHypeTrainContributionType Type = FTwitchSDKHypeTrainContributionType(0);
    /** The total contributed. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Total = 0;
};

/** A Hype Train makes progress on the user's channel. Requires the channel:read:hype_train scope. */
USTRUCT(BlueprintType)
struct FTwitchSDKHypeTrainEvent {
    GENERATED_BODY()

public:
    FTwitchSDKHypeTrainEvent() = default;
    FTwitchSDKHypeTrainEvent(const TwitchSDK::HypeTrainEvent& x) : HypeTrainId(TwitchSDK::ToFString(x.HypeTrainId)), BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), BroadcasterName(TwitchSDK::ToFString(x.BroadcasterName)), Level(x.Level), TotalPoints(x.TotalPoints), Progress(x.Progress), Goal(x.Goal), StartedAt(TwitchSDK::ToFString(x.StartedAt)), ExpiresAt(TwitchSDK::ToFString(x.ExpiresAt)), EndedAt(TwitchSDK::ToFString(x.EndedAt)), CooldownEndsAt(TwitchSDK::ToFString(x.CooldownEndsAt)), TopContributions(TwitchSDK::ToUnrealArray<TwitchSDK::HypeTrainContribution, FTwitchSDKHypeTrainContribution>(x.TopContributions)), LastContribution(x.LastContribution) {}
    operator TwitchSDK::HypeTrainEvent () const {
        TwitchSDK::HypeTrainEvent v;
        v.HypeTrainId = TwitchSDK::FromFString(HypeTrainId);
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.BroadcasterName = TwitchSDK::FromFString(BroadcasterName);
        v.Level = Level;
        v.TotalPoints = TotalPoints;
        v.Progress = Progress;
        v.Goal = Goal;
        v.StartedAt = TwitchSDK::FromFString(StartedAt);
        v.ExpiresAt = TwitchSDK::FromFString(ExpiresAt);
        v.EndedAt = TwitchSDK::FromFString(EndedAt);
        v.CooldownEndsAt = TwitchSDK::FromFString(CooldownEndsAt);
        v.TopContributions = TwitchSDK::FromUnrealArray<TwitchSDK::HypeTrainContribution, FTwitchSDKHypeTrainContribution>(TopContributions);
        v.LastContribution = LastContribution;
        return v;
    }
    /** The Hype Train ID. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString HypeTrainId;
    /** The requested broadcaster ID. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** The requested broadcaster display name. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterName;
    /** The current level of the Hype Train. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 Level = 0;
    /** Total points contributed to the Hype Train. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 TotalPoints = 0;
    /** The number of points contributed to the Hype Train at the current level. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Progress = 0;
    /** The number of points required to reach the next level. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Goal = 0;
    /** The time when the Hype Train started. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString StartedAt;
    /** The time when the Hype Train expires. The expiration is extended when the Hype Train reaches a new level. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ExpiresAt;
    /** The time when the Hype Train ended. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString EndedAt;
    /** The time when the Hype Train cooldown ends so that the next Hype Train can start. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CooldownEndsAt;
    /** The contributors with the most points contributed. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKHypeTrainContribution> TopContributions;
    /** The most recent contribution. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKHypeTrainContribution LastContribution;
};

USTRUCT(BlueprintType)
struct FTwitchSDKCustomRewardDefinition {
    GENERATED_BODY()

public:
    FTwitchSDKCustomRewardDefinition() = default;
    FTwitchSDKCustomRewardDefinition(const TwitchSDK::CustomRewardDefinition& x) : Title(TwitchSDK::ToFString(x.Title)), Cost(x.Cost), Prompt(TwitchSDK::ToFString(x.Prompt)), IsEnabled(x.IsEnabled), BackgroundColor(TwitchSDK::ToFString(x.BackgroundColor)), IsUserInputRequired(x.IsUserInputRequired), IsMaxPerStreamEnabled(x.IsMaxPerStreamEnabled), MaxPerStream(x.MaxPerStream), IsMaxPerUserPerStreamEnabled(x.IsMaxPerUserPerStreamEnabled), MaxPerUserPerStream(x.MaxPerUserPerStream), IsGlobalCooldownEnabled(x.IsGlobalCooldownEnabled), GlobalCooldownSeconds(x.GlobalCooldownSeconds), ShouldRedemptionsSkipRequestQueue(x.ShouldRedemptionsSkipRequestQueue) {}
    operator TwitchSDK::CustomRewardDefinition () const {
        TwitchSDK::CustomRewardDefinition v;
        v.Title = TwitchSDK::FromFString(Title);
        v.Cost = Cost;
        v.Prompt = TwitchSDK::FromFString(Prompt);
        v.IsEnabled = IsEnabled;
        v.BackgroundColor = TwitchSDK::FromFString(BackgroundColor);
        v.IsUserInputRequired = IsUserInputRequired;
        v.IsMaxPerStreamEnabled = IsMaxPerStreamEnabled;
        v.MaxPerStream = MaxPerStream;
        v.IsMaxPerUserPerStreamEnabled = IsMaxPerUserPerStreamEnabled;
        v.MaxPerUserPerStream = MaxPerUserPerStream;
        v.IsGlobalCooldownEnabled = IsGlobalCooldownEnabled;
        v.GlobalCooldownSeconds = GlobalCooldownSeconds;
        v.ShouldRedemptionsSkipRequestQueue = ShouldRedemptionsSkipRequestQueue;
        return v;
    }
    /** The title of the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** The cost of the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Cost = 0;
    /** Optional. The prompt for the viewer when redeeming the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Prompt;
    /** Optional. Is the reward currently enabled, if false the reward won't show up to viewers. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsEnabled = true;
    /** Optional. Custom background color for the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BackgroundColor;
    /** Optional. Does the user need to enter information when redeeming the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsUserInputRequired = false;
    /** Optional. Whether a maximum per stream is enabled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsMaxPerStreamEnabled = false;
    /** Optional. The maximum number per stream if enabled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 MaxPerStream = 0;
    /** Optional. Whether a maximum per user per stream is enabled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsMaxPerUserPerStreamEnabled = false;
    /** Optional. The maximum number per user per stream if enabled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 MaxPerUserPerStream = 0;
    /** Optional. Whether a cooldown is enabled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsGlobalCooldownEnabled = false;
    /** Optional. The cooldown in seconds if enabled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 GlobalCooldownSeconds = 0;
    /** Optional. Should redemptions be set to CustomRewardRedemptionState::Fulfilled status immediately when redeemed and skip the request queue instead of the normal CustomRewardRedemptionState::Unfulfilled status. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool ShouldRedemptionsSkipRequestQueue = false;
};

USTRUCT(BlueprintType)
struct FTwitchSDKCustomRewardList {
    GENERATED_BODY()

public:
    FTwitchSDKCustomRewardList() = default;
    FTwitchSDKCustomRewardList(const TwitchSDK::CustomRewardList& x) : Rewards(TwitchSDK::ToUnrealArray<TwitchSDK::CustomRewardDefinition, FTwitchSDKCustomRewardDefinition>(x.Rewards)) {}
    operator TwitchSDK::CustomRewardList () const {
        TwitchSDK::CustomRewardList v;
        v.Rewards = TwitchSDK::FromUnrealArray<TwitchSDK::CustomRewardDefinition, FTwitchSDKCustomRewardDefinition>(Rewards);
        return v;
    }
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKCustomRewardDefinition> Rewards;
};

/** Describes a single Twitch Drop entitlement granted to the currently logged-in user. */
USTRUCT(BlueprintType)
struct FTwitchSDKDropEntitlement {
    GENERATED_BODY()

public:
    FTwitchSDKDropEntitlement() = default;
    FTwitchSDKDropEntitlement(const TwitchSDK::DropEntitlement& x) : Id(TwitchSDK::ToFString(x.Id)), BenefitId(TwitchSDK::ToFString(x.BenefitId)), Timestamp(TwitchSDK::ToFString(x.Timestamp)), UserId(TwitchSDK::ToFString(x.UserId)), GameId(TwitchSDK::ToFString(x.GameId)), FulfillmentStatus(FTwitchSDKDropFulfillmentStatus(x.FulfillmentStatus)), LastUpdated(TwitchSDK::ToFString(x.LastUpdated)) {}
    operator TwitchSDK::DropEntitlement () const {
        TwitchSDK::DropEntitlement v;
        v.Id = TwitchSDK::FromFString(Id);
        v.BenefitId = TwitchSDK::FromFString(BenefitId);
        v.Timestamp = TwitchSDK::FromFString(Timestamp);
        v.UserId = TwitchSDK::FromFString(UserId);
        v.GameId = TwitchSDK::FromFString(GameId);
        v.FulfillmentStatus = TwitchSDK::DropFulfillmentStatus(FulfillmentStatus);
        v.LastUpdated = TwitchSDK::FromFString(LastUpdated);
        return v;
    }
    /** Unique ID of the entitlement. Pass this to FulfillDropEntitlements once the reward has been granted in-game. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** ID of the reward benefit the user is entitled to. Use this to decide which in-game item to grant. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BenefitId;
    /** RFC3339 timestamp of when the entitlement was granted. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Timestamp;
    /** Twitch user ID the entitlement belongs to (the currently logged-in user). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** ID of the game (category) the entitlement is associated with. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GameId;
    /** Current fulfillment status of the entitlement. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKDropFulfillmentStatus FulfillmentStatus = FTwitchSDKDropFulfillmentStatus(0);
    /** RFC3339 timestamp of when the entitlement was last updated. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString LastUpdated;
};

/** The result of a GetDropEntitlements query. */
USTRUCT(BlueprintType)
struct FTwitchSDKDropEntitlementList {
    GENERATED_BODY()

public:
    FTwitchSDKDropEntitlementList() = default;
    FTwitchSDKDropEntitlementList(const TwitchSDK::DropEntitlementList& x) : Entitlements(TwitchSDK::ToUnrealArray<TwitchSDK::DropEntitlement, FTwitchSDKDropEntitlement>(x.Entitlements)) {}
    operator TwitchSDK::DropEntitlementList () const {
        TwitchSDK::DropEntitlementList v;
        v.Entitlements = TwitchSDK::FromUnrealArray<TwitchSDK::DropEntitlement, FTwitchSDKDropEntitlement>(Entitlements);
        return v;
    }
    /** The matching Drop entitlements for the logged-in user. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKDropEntitlement> Entitlements;
};

/** Parameters for querying the logged-in user's Twitch Drop entitlements. */
USTRUCT(BlueprintType)
struct FTwitchSDKDropEntitlementQuery {
    GENERATED_BODY()

public:
    FTwitchSDKDropEntitlementQuery() = default;
    FTwitchSDKDropEntitlementQuery(const TwitchSDK::DropEntitlementQuery& x) : GameId(TwitchSDK::ToFString(x.GameId)), FulfillmentStatus(FTwitchSDKDropFulfillmentStatus(x.FulfillmentStatus)) {}
    operator TwitchSDK::DropEntitlementQuery () const {
        TwitchSDK::DropEntitlementQuery v;
        v.GameId = TwitchSDK::FromFString(GameId);
        v.FulfillmentStatus = TwitchSDK::DropFulfillmentStatus(FulfillmentStatus);
        return v;
    }
    /** Optional. Restrict results to entitlements for a specific game (category) ID. Leave empty to return entitlements across all of your games. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GameId;
    /** Optional. Restrict results to entitlements with a specific fulfillment status (typically DropFulfillmentStatus::Claimed to find rewards that still need granting). Leave as DropFulfillmentStatus::Unknown to return entitlements of any status. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKDropFulfillmentStatus FulfillmentStatus = FTwitchSDKDropFulfillmentStatus(0);
};

/** Request to mark one or more Drop entitlements as fulfilled. */
USTRUCT(BlueprintType)
struct FTwitchSDKFulfillDropEntitlementsRequest {
    GENERATED_BODY()

public:
    FTwitchSDKFulfillDropEntitlementsRequest() = default;
    FTwitchSDKFulfillDropEntitlementsRequest(const TwitchSDK::FulfillDropEntitlementsRequest& x) : EntitlementIds(TwitchSDK::ToFStringArray(x.EntitlementIds)) {}
    operator TwitchSDK::FulfillDropEntitlementsRequest () const {
        TwitchSDK::FulfillDropEntitlementsRequest v;
        v.EntitlementIds = TwitchSDK::FromFStringArray(EntitlementIds);
        return v;
    }
    /** The IDs of the entitlements to mark as DropFulfillmentStatus::Fulfilled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> EntitlementIds;
};

USTRUCT(BlueprintType)
struct FTwitchSDKBitsLeaderboardEntry {
    GENERATED_BODY()

public:
    FTwitchSDKBitsLeaderboardEntry() = default;
    FTwitchSDKBitsLeaderboardEntry(const TwitchSDK::BitsLeaderboardEntry& x) : UserId(TwitchSDK::ToFString(x.UserId)), UserName(TwitchSDK::ToFString(x.UserName)), Rank(x.Rank), Score(x.Score) {}
    operator TwitchSDK::BitsLeaderboardEntry () const {
        TwitchSDK::BitsLeaderboardEntry v;
        v.UserId = TwitchSDK::FromFString(UserId);
        v.UserName = TwitchSDK::FromFString(UserName);
        v.Rank = Rank;
        v.Score = Score;
        return v;
    }
    /** ID of the user (viewer) in the leaderboard entry. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** Display name corresponding to BitsLeaderboardEntry::UserId. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserName;
    /** Leaderboard rank of the user. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Rank = 0;
    /** Leaderboard score (number of Bits) of the user. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Score = 0;
};

USTRUCT(BlueprintType)
struct FTwitchSDKBitsLeaderboard {
    GENERATED_BODY()

public:
    FTwitchSDKBitsLeaderboard() = default;
    FTwitchSDKBitsLeaderboard(const TwitchSDK::BitsLeaderboard& x) : Data(TwitchSDK::ToUnrealArray<TwitchSDK::BitsLeaderboardEntry, FTwitchSDKBitsLeaderboardEntry>(x.Data)), StartedAt(TwitchSDK::ToFString(x.StartedAt)), EndedAt(TwitchSDK::ToFString(x.EndedAt)) {}
    operator TwitchSDK::BitsLeaderboard () const {
        TwitchSDK::BitsLeaderboard v;
        v.Data = TwitchSDK::FromUnrealArray<TwitchSDK::BitsLeaderboardEntry, FTwitchSDKBitsLeaderboardEntry>(Data);
        v.StartedAt = TwitchSDK::FromFString(StartedAt);
        v.EndedAt = TwitchSDK::FromFString(EndedAt);
        return v;
    }
    /** Entries on the bits leaderboard, in no particular order. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKBitsLeaderboardEntry> Data;
    /** Start of the date range for the returned data. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString StartedAt;
    /** End of the date range for the returned data. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString EndedAt;
};

USTRUCT(BlueprintType)
struct FTwitchSDKBitsLeaderboardRequest {
    GENERATED_BODY()

public:
    FTwitchSDKBitsLeaderboardRequest() = default;
    FTwitchSDKBitsLeaderboardRequest(const TwitchSDK::BitsLeaderboardRequest& x) : Count(x.Count), Period(TwitchSDK::ToFString(x.Period)), StartedAt(TwitchSDK::ToFString(x.StartedAt)), UserId(TwitchSDK::ToFString(x.UserId)) {}
    operator TwitchSDK::BitsLeaderboardRequest () const {
        TwitchSDK::BitsLeaderboardRequest v;
        v.Count = Count;
        v.Period = TwitchSDK::FromFString(Period);
        v.StartedAt = TwitchSDK::FromFString(StartedAt);
        v.UserId = TwitchSDK::FromFString(UserId);
        return v;
    }
    /** Number of results to be returned. Maximum: 100. Default: 10. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 Count = 0;
    /** Time period over which data is aggregated (PST time zone). This parameter interacts with BitsLeaderboardRequest::StartedAt. Default: all. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Period;
    /** Timestamp for the period over which the returned data is aggregated. Must be in RFC 3339 format. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString StartedAt;
    /** ID of the user whose results are returned; i.e., the person who paid for the Bits. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
};

/** A specified channel receives a follow. */
USTRUCT(BlueprintType)
struct FTwitchSDKChannelFollowEvent {
    GENERATED_BODY()

public:
    FTwitchSDKChannelFollowEvent() = default;
    FTwitchSDKChannelFollowEvent(const TwitchSDK::ChannelFollowEvent& x) : UserId(TwitchSDK::ToFString(x.UserId)), UserDisplayName(TwitchSDK::ToFString(x.UserDisplayName)), FollowedAt(TwitchSDK::ToFString(x.FollowedAt)) {}
    operator TwitchSDK::ChannelFollowEvent () const {
        TwitchSDK::ChannelFollowEvent v;
        v.UserId = TwitchSDK::FromFString(UserId);
        v.UserDisplayName = TwitchSDK::FromFString(UserDisplayName);
        v.FollowedAt = TwitchSDK::FromFString(FollowedAt);
        return v;
    }
    /** The user ID for the user now following the specified channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** The user display name for the user now following the specified channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserDisplayName;
    /** RFC3339 timestamp of when the follow occurred. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString FollowedAt;
};

/** A user cheers on the specified channel. */
USTRUCT(BlueprintType)
struct FTwitchSDKChannelCheerEvent {
    GENERATED_BODY()

public:
    FTwitchSDKChannelCheerEvent() = default;
    FTwitchSDKChannelCheerEvent(const TwitchSDK::ChannelCheerEvent& x) : IsAnonymous(x.IsAnonymous), UserId(TwitchSDK::ToFString(x.UserId)), UserDisplayName(TwitchSDK::ToFString(x.UserDisplayName)), Message(TwitchSDK::ToFString(x.Message)), Bits(x.Bits) {}
    operator TwitchSDK::ChannelCheerEvent () const {
        TwitchSDK::ChannelCheerEvent v;
        v.IsAnonymous = IsAnonymous;
        v.UserId = TwitchSDK::FromFString(UserId);
        v.UserDisplayName = TwitchSDK::FromFString(UserDisplayName);
        v.Message = TwitchSDK::FromFString(Message);
        v.Bits = Bits;
        return v;
    }
    /** Whether the user cheered anonymously or not. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsAnonymous = false;
    /** The user ID for the user who cheered on the specified channel. This is null if ChannelCheerEvent::IsAnonymous is true. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** The user display name for the user who cheered on the specified channel. This is null if ChannelCheerEvent::IsAnonymous is true. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserDisplayName;
    /** The message sent with the cheer. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Message;
    /** The number of bits cheered. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Bits = 0;
};

/** A viewer has redeemed a custom channel points reward on the specified channel or the redemption has been updated (i.e., fulfilled or cancelled). */
USTRUCT(BlueprintType)
struct FTwitchSDKCustomRewardEvent {
    GENERATED_BODY()

public:
    FTwitchSDKCustomRewardEvent() = default;
    FTwitchSDKCustomRewardEvent(const TwitchSDK::CustomRewardEvent& x) : RedemptionId(TwitchSDK::ToFString(x.RedemptionId)), BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), RedeemerId(TwitchSDK::ToFString(x.RedeemerId)), BroadcasterName(TwitchSDK::ToFString(x.BroadcasterName)), RedeemerName(TwitchSDK::ToFString(x.RedeemerName)), UserInput(TwitchSDK::ToFString(x.UserInput)), Status(FTwitchSDKCustomRewardRedemptionState(x.Status)), CustomRewardId(TwitchSDK::ToFString(x.CustomRewardId)), CustomRewardTitle(TwitchSDK::ToFString(x.CustomRewardTitle)), CustomRewardCost(x.CustomRewardCost), CustomRewardPrompt(TwitchSDK::ToFString(x.CustomRewardPrompt)), RedeemedAt(TwitchSDK::ToFString(x.RedeemedAt)) {}
    operator TwitchSDK::CustomRewardEvent () const {
        TwitchSDK::CustomRewardEvent v;
        v.RedemptionId = TwitchSDK::FromFString(RedemptionId);
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.RedeemerId = TwitchSDK::FromFString(RedeemerId);
        v.BroadcasterName = TwitchSDK::FromFString(BroadcasterName);
        v.RedeemerName = TwitchSDK::FromFString(RedeemerName);
        v.UserInput = TwitchSDK::FromFString(UserInput);
        v.Status = TwitchSDK::CustomRewardRedemptionState(Status);
        v.CustomRewardId = TwitchSDK::FromFString(CustomRewardId);
        v.CustomRewardTitle = TwitchSDK::FromFString(CustomRewardTitle);
        v.CustomRewardCost = CustomRewardCost;
        v.CustomRewardPrompt = TwitchSDK::FromFString(CustomRewardPrompt);
        v.RedeemedAt = TwitchSDK::FromFString(RedeemedAt);
        return v;
    }
    /** The redemption identifier. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString RedemptionId;
    /** The requested broadcaster ID. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** User ID of the user that redeemed the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString RedeemerId;
    /** The requested broadcaster display name. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterName;
    /** Display name of the user that redeemed the reward. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString RedeemerName;
    /** The user input provided. Empty string if not provided. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserInput;
    /** Status of the redemption. If you resolve or cancel all redemptions from your code, you should discard all events of this type with a status other than CustomRewardRedemptionState::Unfulfilled. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKCustomRewardRedemptionState Status = FTwitchSDKCustomRewardRedemptionState(0);
    /** The reward identifier. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CustomRewardId;
    /** The reward name. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CustomRewardTitle;
    /** The reward cost. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 CustomRewardCost = 0;
    /** The reward description. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CustomRewardPrompt;
    /** RFC3339 timestamp of when the reward was redeemed. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString RedeemedAt;
};

USTRUCT(BlueprintType)
struct FTwitchSDKEventStreamRequest {
    GENERATED_BODY()

public:
    FTwitchSDKEventStreamRequest() = default;
    FTwitchSDKEventStreamRequest(const TwitchSDK::EventStreamRequest& x) : Kind(FTwitchSDKEventStreamKind(x.Kind)) {}
    operator TwitchSDK::EventStreamRequest () const {
        TwitchSDK::EventStreamRequest v;
        v.Kind = TwitchSDK::EventStreamKind(Kind);
        return v;
    }
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKEventStreamKind Kind = FTwitchSDKEventStreamKind(0);
};

USTRUCT(BlueprintType)
struct FTwitchSDKEventStreamDesc {
    GENERATED_BODY()

public:
    FTwitchSDKEventStreamDesc() = default;
    FTwitchSDKEventStreamDesc(const TwitchSDK::EventStreamDesc& x) : Kind(FTwitchSDKEventStreamKind(x.Kind)), Token(x.Token) {}
    operator TwitchSDK::EventStreamDesc () const {
        TwitchSDK::EventStreamDesc v;
        v.Kind = TwitchSDK::EventStreamKind(Kind);
        v.Token = Token;
        return v;
    }
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKEventStreamKind Kind = FTwitchSDKEventStreamKind(0);
    /** Opaque token identifying the subscription. Do not touch. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 Token = 0;
};

USTRUCT(BlueprintType)
struct FTwitchSDKModifyChannelInfoRequest {
    GENERATED_BODY()

public:
    FTwitchSDKModifyChannelInfoRequest() = default;
    FTwitchSDKModifyChannelInfoRequest(const TwitchSDK::ModifyChannelInfoRequest& x) : GameId(TwitchSDK::ToFString(x.GameId)), Language(TwitchSDK::ToFString(x.Language)), Title(TwitchSDK::ToFString(x.Title)), Delay(x.Delay), Tags(TwitchSDK::ToFStringArray(x.Tags)), ForceUpdateTags(x.ForceUpdateTags) {}
    operator TwitchSDK::ModifyChannelInfoRequest () const {
        TwitchSDK::ModifyChannelInfoRequest v;
        v.GameId = TwitchSDK::FromFString(GameId);
        v.Language = TwitchSDK::FromFString(Language);
        v.Title = TwitchSDK::FromFString(Title);
        v.Delay = Delay;
        v.Tags = TwitchSDK::FromFStringArray(Tags);
        v.ForceUpdateTags = ForceUpdateTags;
        return v;
    }
    /** The current game ID being played on the channel. Use "0" to unset the game. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GameId;
    /** The language of the channel. A language value must be either the ISO 639-1 two-letter code for a supported stream language or "other". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Language;
    /** The title of the stream. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Stream delay in seconds. Stream delay is a Twitch Partner feature; trying to set this value for other account types will return a 400 error. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 Delay = -1;
    /** A list of channel-defined tags to apply to the channel. Tags help identify the content that the channel streams. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> Tags;
    /** If ModifyChannelInfoRequest::Tags is missing or empty, channel tags will not be altered by default (if ModifyChannelInfoRequest::ForceUpdateTags is false). If ModifyChannelInfoRequest::ForceUpdateTags is true, passing an empty (or missing) array ModifyChannelInfoRequest::Tags causes all tags to be removed from the channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool ForceUpdateTags = false;
};

/** Contains the information necessary to create a new prediction. */
USTRUCT(BlueprintType)
struct FTwitchSDKPredictionDefinition {
    GENERATED_BODY()

public:
    FTwitchSDKPredictionDefinition() = default;
    FTwitchSDKPredictionDefinition(const TwitchSDK::PredictionDefinition& x) : Title(TwitchSDK::ToFString(x.Title)), Outcomes(TwitchSDK::ToFStringArray(x.Outcomes)), Duration(x.Duration) {}
    operator TwitchSDK::PredictionDefinition () const {
        TwitchSDK::PredictionDefinition v;
        v.Title = TwitchSDK::FromFString(Title);
        v.Outcomes = TwitchSDK::FromFStringArray(Outcomes);
        v.Duration = Duration;
        return v;
    }
    /** Title for the Prediction. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Array of outcome titles for the Prediction. Array size must be 2. The first outcome object is the "blue" outcome and the second outcome object is the "pink" outcome when viewing the Prediction on Twitch. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> Outcomes;
    /** Total duration for the Prediction (in seconds). Minimum: 1. Maximum: 1800. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 Duration = 0;
};

USTRUCT(BlueprintType)
struct FTwitchSDKPredictionOutcome {
    GENERATED_BODY()

public:
    FTwitchSDKPredictionOutcome() = default;
    FTwitchSDKPredictionOutcome(const TwitchSDK::PredictionOutcome& x) : Id(TwitchSDK::ToFString(x.Id)), Title(TwitchSDK::ToFString(x.Title)), Users(x.Users), ChannelPoints(x.ChannelPoints), Color(TwitchSDK::ToFString(x.Color)) {}
    operator TwitchSDK::PredictionOutcome () const {
        TwitchSDK::PredictionOutcome v;
        v.Id = TwitchSDK::FromFString(Id);
        v.Title = TwitchSDK::FromFString(Title);
        v.Users = Users;
        v.ChannelPoints = ChannelPoints;
        v.Color = TwitchSDK::FromFString(Color);
        return v;
    }
    /** ID for the outcome. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** Text displayed for outcome. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Number of unique users that chose the outcome. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Users = 0;
    /** Number of Channel Points used for the outcome. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 ChannelPoints = 0;
    /** Color for the outcome. Valid values: BLUE, PINK */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Color;
};

/** Describes a Channel Points Predictions on a Twitch channel. */
USTRUCT(BlueprintType)
struct FTwitchSDKPredictionInfo {
    GENERATED_BODY()

public:
    FTwitchSDKPredictionInfo() = default;
    FTwitchSDKPredictionInfo(const TwitchSDK::PredictionInfo& x) : Id(TwitchSDK::ToFString(x.Id)), BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), Title(TwitchSDK::ToFString(x.Title)), WinningOutcomeId(TwitchSDK::ToFString(x.WinningOutcomeId)), Outcomes(TwitchSDK::ToUnrealArray<TwitchSDK::PredictionOutcome, FTwitchSDKPredictionOutcome>(x.Outcomes)), Status(FTwitchSDKPredictionStatus(x.Status)), CreatedAt(TwitchSDK::ToFString(x.CreatedAt)), EndedAt(TwitchSDK::ToFString(x.EndedAt)), LockedAt(TwitchSDK::ToFString(x.LockedAt)) {}
    operator TwitchSDK::PredictionInfo () const {
        TwitchSDK::PredictionInfo v;
        v.Id = TwitchSDK::FromFString(Id);
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.Title = TwitchSDK::FromFString(Title);
        v.WinningOutcomeId = TwitchSDK::FromFString(WinningOutcomeId);
        v.Outcomes = TwitchSDK::FromUnrealArray<TwitchSDK::PredictionOutcome, FTwitchSDKPredictionOutcome>(Outcomes);
        v.Status = TwitchSDK::PredictionStatus(Status);
        v.CreatedAt = TwitchSDK::FromFString(CreatedAt);
        v.EndedAt = TwitchSDK::FromFString(EndedAt);
        v.LockedAt = TwitchSDK::FromFString(LockedAt);
        return v;
    }
    /** ID of the Prediction. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** ID of the broadcaster. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** Title for the Prediction. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** ID of the winning outcome. If PredictionInfo::Status is PredictionStatus::Active, this is set to null. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString WinningOutcomeId;
    /** Array of possible outcomes for the Prediction. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKPredictionOutcome> Outcomes;
    /** Status of the Prediction. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKPredictionStatus Status = FTwitchSDKPredictionStatus(0);
    /** UTC timestamp for the Prediction's start time. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CreatedAt;
    /** UTC timestamp for when the Prediction ended. If PredictionInfo::Status is PredictionStatus::Active, this is set to null. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString EndedAt;
    /** UTC timestamp for when the Prediction was locked. If PredictionInfo::Status is PredictionStatus::Active, this is set to null. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString LockedAt;
};

USTRUCT(BlueprintType)
struct FTwitchSDKEndPredictionRequest {
    GENERATED_BODY()

public:
    FTwitchSDKEndPredictionRequest() = default;
    FTwitchSDKEndPredictionRequest(const TwitchSDK::EndPredictionRequest& x) : BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), PredictionId(TwitchSDK::ToFString(x.PredictionId)), Status(FTwitchSDKPredictionStatus(x.Status)), WinningOutcomeId(TwitchSDK::ToFString(x.WinningOutcomeId)) {}
    operator TwitchSDK::EndPredictionRequest () const {
        TwitchSDK::EndPredictionRequest v;
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.PredictionId = TwitchSDK::FromFString(PredictionId);
        v.Status = TwitchSDK::PredictionStatus(Status);
        v.WinningOutcomeId = TwitchSDK::FromFString(WinningOutcomeId);
        return v;
    }
    /** The broadcaster running prediction events. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** ID of the Prediction. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString PredictionId;
    /** The Prediction status to be set. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKPredictionStatus Status = FTwitchSDKPredictionStatus(0);
    /** ID of the winning outcome for the Prediction, if EndPredictionRequest::Status is being set to PredictionStatus::Resolved. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString WinningOutcomeId;
};

/** Describes a Stream Marker. */
USTRUCT(BlueprintType)
struct FTwitchSDKStreamMarkerInfo {
    GENERATED_BODY()

public:
    FTwitchSDKStreamMarkerInfo() = default;
    FTwitchSDKStreamMarkerInfo(const TwitchSDK::StreamMarkerInfo& x) : Id(TwitchSDK::ToFString(x.Id)), CreatedAt(TwitchSDK::ToFString(x.CreatedAt)), Description(TwitchSDK::ToFString(x.Description)), PositionSeconds(x.PositionSeconds) {}
    operator TwitchSDK::StreamMarkerInfo () const {
        TwitchSDK::StreamMarkerInfo v;
        v.Id = TwitchSDK::FromFString(Id);
        v.CreatedAt = TwitchSDK::FromFString(CreatedAt);
        v.Description = TwitchSDK::FromFString(Description);
        v.PositionSeconds = PositionSeconds;
        return v;
    }
    /** Unique ID of the marker. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** RFC3339 timestamp of the marker. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CreatedAt;
    /** Description of the marker. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Description;
    /** Relative offset (in seconds) of the marker, from the beginning of the stream. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 PositionSeconds = 0;
};

/** Describes a Clip. */
USTRUCT(BlueprintType)
struct FTwitchSDKClipInfo {
    GENERATED_BODY()

public:
    FTwitchSDKClipInfo() = default;
    FTwitchSDKClipInfo(const TwitchSDK::ClipInfo& x) : Id(TwitchSDK::ToFString(x.Id)), Url(TwitchSDK::ToFString(x.Url)), EmbedUrl(TwitchSDK::ToFString(x.EmbedUrl)), CreatedAt(TwitchSDK::ToFString(x.CreatedAt)), ThumbnailUrl(TwitchSDK::ToFString(x.ThumbnailUrl)), Duration(x.Duration) {}
    operator TwitchSDK::ClipInfo () const {
        TwitchSDK::ClipInfo v;
        v.Id = TwitchSDK::FromFString(Id);
        v.Url = TwitchSDK::FromFString(Url);
        v.EmbedUrl = TwitchSDK::FromFString(EmbedUrl);
        v.CreatedAt = TwitchSDK::FromFString(CreatedAt);
        v.ThumbnailUrl = TwitchSDK::FromFString(ThumbnailUrl);
        v.Duration = Duration;
        return v;
    }
    /** ID of the clip being queried. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** URL where the clip can be viewed. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Url;
    /** URL to embed the clip. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString EmbedUrl;
    /** Date when the clip was created. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CreatedAt;
    /** URL of the clip thumbnail. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ThumbnailUrl;
    /** Duration of the Clip in seconds. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 Duration = 0;
};

USTRUCT(BlueprintType)
struct FTwitchSDKEndPollRequest {
    GENERATED_BODY()

public:
    FTwitchSDKEndPollRequest() = default;
    FTwitchSDKEndPollRequest(const TwitchSDK::EndPollRequest& x) : BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), PollId(TwitchSDK::ToFString(x.PollId)), ShowResults(x.ShowResults) {}
    operator TwitchSDK::EndPollRequest () const {
        TwitchSDK::EndPollRequest v;
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.PollId = TwitchSDK::FromFString(PollId);
        v.ShowResults = ShowResults;
        return v;
    }
    /** ID of the broadcaster. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** ID of the poll. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString PollId;
    /** If true, sets the poll status to PollStatus::Terminated so that results are shown. If false, sets the poll status to PollStatus::Archived so that the poll is canceled immediately and no results are shown. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool ShowResults = false;
};

USTRUCT(BlueprintType)
struct FTwitchSDKPollChoiceInfo {
    GENERATED_BODY()

public:
    FTwitchSDKPollChoiceInfo() = default;
    FTwitchSDKPollChoiceInfo(const TwitchSDK::PollChoiceInfo& x) : Id(TwitchSDK::ToFString(x.Id)), Title(TwitchSDK::ToFString(x.Title)), Votes(x.Votes), ChannelPointsVotes(x.ChannelPointsVotes) {}
    operator TwitchSDK::PollChoiceInfo () const {
        TwitchSDK::PollChoiceInfo v;
        v.Id = TwitchSDK::FromFString(Id);
        v.Title = TwitchSDK::FromFString(Title);
        v.Votes = Votes;
        v.ChannelPointsVotes = ChannelPointsVotes;
        return v;
    }
    /** ID for the choice. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** Text displayed for the choice. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Total number of votes received for the choice across all methods of voting. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Votes = 0;
    /** Number of votes received via Channel Points. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 ChannelPointsVotes = 0;
};

/** Describes a Poll on a Twitch channel. */
USTRUCT(BlueprintType)
struct FTwitchSDKPollInfo {
    GENERATED_BODY()

public:
    FTwitchSDKPollInfo() = default;
    FTwitchSDKPollInfo(const TwitchSDK::PollInfo& x) : Id(TwitchSDK::ToFString(x.Id)), BroadcasterId(TwitchSDK::ToFString(x.BroadcasterId)), Title(TwitchSDK::ToFString(x.Title)), Choices(TwitchSDK::ToUnrealArray<TwitchSDK::PollChoiceInfo, FTwitchSDKPollChoiceInfo>(x.Choices)), ChannelPointsVotingEnabled(x.ChannelPointsVotingEnabled), ChannelPointsPerVote(x.ChannelPointsPerVote), Status(FTwitchSDKPollStatus(x.Status)), StartedAt(TwitchSDK::ToFString(x.StartedAt)), EndedAt(TwitchSDK::ToFString(x.EndedAt)) {}
    operator TwitchSDK::PollInfo () const {
        TwitchSDK::PollInfo v;
        v.Id = TwitchSDK::FromFString(Id);
        v.BroadcasterId = TwitchSDK::FromFString(BroadcasterId);
        v.Title = TwitchSDK::FromFString(Title);
        v.Choices = TwitchSDK::FromUnrealArray<TwitchSDK::PollChoiceInfo, FTwitchSDKPollChoiceInfo>(Choices);
        v.ChannelPointsVotingEnabled = ChannelPointsVotingEnabled;
        v.ChannelPointsPerVote = ChannelPointsPerVote;
        v.Status = TwitchSDK::PollStatus(Status);
        v.StartedAt = TwitchSDK::FromFString(StartedAt);
        v.EndedAt = TwitchSDK::FromFString(EndedAt);
        return v;
    }
    /** ID of the poll. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** ID of the broadcaster. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterId;
    /** Question displayed for the poll. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Array of the poll choices. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKPollChoiceInfo> Choices;
    /** Indicates if Channel Points can be used for voting. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool ChannelPointsVotingEnabled = false;
    /** Number of Channel Points required to vote once with Channel Points. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 ChannelPointsPerVote = 0;
    /** Poll status. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FTwitchSDKPollStatus Status = FTwitchSDKPollStatus(0);
    /** UTC timestamp for the poll's start time. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString StartedAt;
    /** UTC timestamp for the poll's end time. Set to null if the poll is still PollStatus::Active. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString EndedAt;
};

/** Describes a query for streams. All fields are optional. */
USTRUCT(BlueprintType)
struct FTwitchSDKStreamQuery {
    GENERATED_BODY()

public:
    FTwitchSDKStreamQuery() = default;
    FTwitchSDKStreamQuery(const TwitchSDK::StreamQuery& x) : After(TwitchSDK::ToFString(x.After)), First(x.First), GameIds(TwitchSDK::ToFStringArray(x.GameIds)), Languages(TwitchSDK::ToFStringArray(x.Languages)), UserIds(TwitchSDK::ToFStringArray(x.UserIds)), UserLogins(TwitchSDK::ToFStringArray(x.UserLogins)) {}
    operator TwitchSDK::StreamQuery () const {
        TwitchSDK::StreamQuery v;
        v.After = TwitchSDK::FromFString(After);
        v.First = First;
        v.GameIds = TwitchSDK::FromFStringArray(GameIds);
        v.Languages = TwitchSDK::FromFStringArray(Languages);
        v.UserIds = TwitchSDK::FromFStringArray(UserIds);
        v.UserLogins = TwitchSDK::FromFStringArray(UserLogins);
        return v;
    }
    /** Cursor for forward pagination: tells the server where to start fetching the next set of results, in a multi-page response. The cursor value specified here is from the PaginationCursor response field of a prior query. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString After;
    /** Maximum number of objects to return. Maximum: 100. Default: 20. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 First = 0;
    /** Returns streams broadcasting a specified game ID. You can specify up to 100 IDs. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> GameIds;
    /** Stream language. You can specify up to 100 languages. A language value must be either the ISO 639-1 two-letter code for a supported stream language or "other". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> Languages;
    /** Returns streams broadcast by one or more specified user IDs. You can specify up to 100 IDs. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> UserIds;
    /** Returns streams broadcast by one or more specified user login names. You can specify up to 100 names. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> UserLogins;
};

USTRUCT(BlueprintType)
struct FTwitchSDKWebRequestResult {
    GENERATED_BODY()

public:
    FTwitchSDKWebRequestResult() = default;
    FTwitchSDKWebRequestResult(const TwitchSDK::WebRequestResult& x) : HttpStatus(x.HttpStatus), ResponseBody(TwitchSDK::ToFString(x.ResponseBody)) {}
    operator TwitchSDK::WebRequestResult () const {
        TwitchSDK::WebRequestResult v;
        v.HttpStatus = HttpStatus;
        v.ResponseBody = TwitchSDK::FromFString(ResponseBody);
        return v;
    }
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 HttpStatus = 0;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ResponseBody;
};

USTRUCT(BlueprintType)
struct FTwitchSDKAuthenticationInfo {
    GENERATED_BODY()

public:
    FTwitchSDKAuthenticationInfo() = default;
    FTwitchSDKAuthenticationInfo(const TwitchSDK::AuthenticationInfo& x) : Uri(TwitchSDK::ToFString(x.Uri)), UserCode(TwitchSDK::ToFString(x.UserCode)) {}
    operator TwitchSDK::AuthenticationInfo () const {
        TwitchSDK::AuthenticationInfo v;
        v.Uri = TwitchSDK::FromFString(Uri);
        v.UserCode = TwitchSDK::FromFString(UserCode);
        return v;
    }
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Uri;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserCode;
};

USTRUCT(BlueprintType)
struct FTwitchSDKUserInfo {
    GENERATED_BODY()

public:
    FTwitchSDKUserInfo() = default;
    FTwitchSDKUserInfo(const TwitchSDK::UserInfo& x) : ChannelId(TwitchSDK::ToFString(x.ChannelId)), LoginName(TwitchSDK::ToFString(x.LoginName)), DisplayName(TwitchSDK::ToFString(x.DisplayName)), UserType(TwitchSDK::ToFString(x.UserType)), BroadcasterType(TwitchSDK::ToFString(x.BroadcasterType)), Description(TwitchSDK::ToFString(x.Description)), ProfileImageUrl(TwitchSDK::ToFString(x.ProfileImageUrl)), OfflineImageUrl(TwitchSDK::ToFString(x.OfflineImageUrl)), ViewCount(x.ViewCount), CreatedAt(TwitchSDK::ToFString(x.CreatedAt)), Email(TwitchSDK::ToFString(x.Email)) {}
    operator TwitchSDK::UserInfo () const {
        TwitchSDK::UserInfo v;
        v.ChannelId = TwitchSDK::FromFString(ChannelId);
        v.LoginName = TwitchSDK::FromFString(LoginName);
        v.DisplayName = TwitchSDK::FromFString(DisplayName);
        v.UserType = TwitchSDK::FromFString(UserType);
        v.BroadcasterType = TwitchSDK::FromFString(BroadcasterType);
        v.Description = TwitchSDK::FromFString(Description);
        v.ProfileImageUrl = TwitchSDK::FromFString(ProfileImageUrl);
        v.OfflineImageUrl = TwitchSDK::FromFString(OfflineImageUrl);
        v.ViewCount = ViewCount;
        v.CreatedAt = TwitchSDK::FromFString(CreatedAt);
        v.Email = TwitchSDK::FromFString(Email);
        return v;
    }
    /** User's ID. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ChannelId;
    /** User's login name. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString LoginName;
    /** User's display name. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString DisplayName;
    /** User's type: "staff", "admin", "global_mod", or "". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserType;
    /** User's broadcaster type: "partner", "affiliate", or "". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString BroadcasterType;
    /** User's channel description. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Description;
    /** URL of the user's profile image. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ProfileImageUrl;
    /** URL of the user's offline image. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString OfflineImageUrl;
    /** Total number of views of the user's channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 ViewCount = 0;
    /** Date when the user was created. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString CreatedAt;
    /** User's verified email address. Only available if you have the user:read:email scope. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Email;
};

/** Describes an active stream on Twitch. */
USTRUCT(BlueprintType)
struct FTwitchSDKStreamInfo {
    GENERATED_BODY()

public:
    FTwitchSDKStreamInfo() = default;
    FTwitchSDKStreamInfo(const TwitchSDK::StreamInfo& x) : Id(TwitchSDK::ToFString(x.Id)), UserId(TwitchSDK::ToFString(x.UserId)), UserLogin(TwitchSDK::ToFString(x.UserLogin)), UserName(TwitchSDK::ToFString(x.UserName)), GameId(TwitchSDK::ToFString(x.GameId)), GameName(TwitchSDK::ToFString(x.GameName)), Type(TwitchSDK::ToFString(x.Type)), Title(TwitchSDK::ToFString(x.Title)), ViewerCount(x.ViewerCount), StartedAt(TwitchSDK::ToFString(x.StartedAt)), Language(TwitchSDK::ToFString(x.Language)), ThumbnailUrl(TwitchSDK::ToFString(x.ThumbnailUrl)), Tags(TwitchSDK::ToFStringArray(x.Tags)), IsMature(x.IsMature) {}
    operator TwitchSDK::StreamInfo () const {
        TwitchSDK::StreamInfo v;
        v.Id = TwitchSDK::FromFString(Id);
        v.UserId = TwitchSDK::FromFString(UserId);
        v.UserLogin = TwitchSDK::FromFString(UserLogin);
        v.UserName = TwitchSDK::FromFString(UserName);
        v.GameId = TwitchSDK::FromFString(GameId);
        v.GameName = TwitchSDK::FromFString(GameName);
        v.Type = TwitchSDK::FromFString(Type);
        v.Title = TwitchSDK::FromFString(Title);
        v.ViewerCount = ViewerCount;
        v.StartedAt = TwitchSDK::FromFString(StartedAt);
        v.Language = TwitchSDK::FromFString(Language);
        v.ThumbnailUrl = TwitchSDK::FromFString(ThumbnailUrl);
        v.Tags = TwitchSDK::FromFStringArray(Tags);
        v.IsMature = IsMature;
        return v;
    }
    /** Stream ID. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Id;
    /** ID of the user who is streaming. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** Login of the user who is streaming. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserLogin;
    /** Display name of the user who is streaming. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserName;
    /** ID of the game being played on the stream. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GameId;
    /** Name of the game being played. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GameName;
    /** Stream type: "live" or "" (in case of error). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Type;
    /** Stream title. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Number of viewers watching the stream at the time of the query. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 ViewerCount = 0;
    /** UTC timestamp. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString StartedAt;
    /** Stream language. A language value is either the ISO 639-1 two-letter code for a supported stream language or "other". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Language;
    /** Thumbnail URL of the stream. All image URLs have variable width and height. You can replace {width} and {height} with any values to get that size image */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString ThumbnailUrl;
    /** Shows tag IDs that apply to the stream. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> Tags;
    /** Indicates if the broadcaster has specified their channel contains mature content that may be inappropriate for younger audiences. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsMature = false;
};

USTRUCT(BlueprintType)
struct FTwitchSDKStreamQueryResult {
    GENERATED_BODY()

public:
    FTwitchSDKStreamQueryResult() = default;
    FTwitchSDKStreamQueryResult(const TwitchSDK::StreamQueryResult& x) : Streams(TwitchSDK::ToUnrealArray<TwitchSDK::StreamInfo, FTwitchSDKStreamInfo>(x.Streams)), PaginationCursor(TwitchSDK::ToFString(x.PaginationCursor)) {}
    operator TwitchSDK::StreamQueryResult () const {
        TwitchSDK::StreamQueryResult v;
        v.Streams = TwitchSDK::FromUnrealArray<TwitchSDK::StreamInfo, FTwitchSDKStreamInfo>(Streams);
        v.PaginationCursor = TwitchSDK::FromFString(PaginationCursor);
        return v;
    }
    /** List of streams matching the query. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FTwitchSDKStreamInfo> Streams;
    /** Pagination cursor to fetch the next page of results. May be empty if there are no further results. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString PaginationCursor;
};

/** A notification when a specified channel receives a subscriber. This does not include resubscribes. */
USTRUCT(BlueprintType)
struct FTwitchSDKChannelSubscribeEvent {
    GENERATED_BODY()

public:
    FTwitchSDKChannelSubscribeEvent() = default;
    FTwitchSDKChannelSubscribeEvent(const TwitchSDK::ChannelSubscribeEvent& x) : UserId(TwitchSDK::ToFString(x.UserId)), UserLogin(TwitchSDK::ToFString(x.UserLogin)), UserDisplayName(TwitchSDK::ToFString(x.UserDisplayName)), Tier(TwitchSDK::ToFString(x.Tier)), IsGift(x.IsGift), CumulativeMonths(x.CumulativeMonths), StreakMonths(x.StreakMonths), DurationMonths(x.DurationMonths) {}
    operator TwitchSDK::ChannelSubscribeEvent () const {
        TwitchSDK::ChannelSubscribeEvent v;
        v.UserId = TwitchSDK::FromFString(UserId);
        v.UserLogin = TwitchSDK::FromFString(UserLogin);
        v.UserDisplayName = TwitchSDK::FromFString(UserDisplayName);
        v.Tier = TwitchSDK::FromFString(Tier);
        v.IsGift = IsGift;
        v.CumulativeMonths = CumulativeMonths;
        v.StreakMonths = StreakMonths;
        v.DurationMonths = DurationMonths;
        return v;
    }
    /** The user ID for the user who subscribed to the specified channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserId;
    /** The user login for the user who subscribed to the specified channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserLogin;
    /** The user display name for the user who subscribed to the specified channel. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString UserDisplayName;
    /** The tier of the subscription. Valid values are 1000, 2000, and 3000. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Tier;
    /** Whether the subscription is a gift. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsGift = false;
    /** The total number of months the user has been subscribed to the channel. This is nonzero only for resubscriptions. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 CumulativeMonths = 0;
    /** The number of consecutive months the user's current subscription has been active. This value is -1 if the user has opted out of sharing this information. This is nonzero only for resubscriptions. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 StreakMonths = 0;
    /** The month duration of the subscription. This is nonzero only for resubscriptions. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 DurationMonths = 0;
};

/** Describes a user's subscription to a specified channel. */
USTRUCT(BlueprintType)
struct FTwitchSDKUserSubscriptionCheckResult {
    GENERATED_BODY()

public:
    FTwitchSDKUserSubscriptionCheckResult() = default;
    FTwitchSDKUserSubscriptionCheckResult(const TwitchSDK::UserSubscriptionCheckResult& x) : Tier(TwitchSDK::ToFString(x.Tier)), GifterLogin(TwitchSDK::ToFString(x.GifterLogin)), GifterName(TwitchSDK::ToFString(x.GifterName)), IsGift(x.IsGift), IsSubscribed(x.IsSubscribed) {}
    operator TwitchSDK::UserSubscriptionCheckResult () const {
        TwitchSDK::UserSubscriptionCheckResult v;
        v.Tier = TwitchSDK::FromFString(Tier);
        v.GifterLogin = TwitchSDK::FromFString(GifterLogin);
        v.GifterName = TwitchSDK::FromFString(GifterName);
        v.IsGift = IsGift;
        v.IsSubscribed = IsSubscribed;
        return v;
    }
    /** Subscription tier. 1000 is tier 1, 2000 is tier 2, and 3000 is tier 3. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Tier;
    /** Login of the gifter (if UserSubscriptionCheckResult::IsGift is true). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GifterLogin;
    /** Display name of the gifter (if UserSubscriptionCheckResult::IsGift is true). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString GifterName;
    /** Indicates if the subscription is a gift. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsGift = false;
    /** Indicates if the user is subscribed at all. If this is false, all other fields are empty. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool IsSubscribed = false;
};

/** Contains the information necessary to create a new poll. */
USTRUCT(BlueprintType)
struct FTwitchSDKPollDefinition {
    GENERATED_BODY()

public:
    FTwitchSDKPollDefinition() = default;
    FTwitchSDKPollDefinition(const TwitchSDK::PollDefinition& x) : Title(TwitchSDK::ToFString(x.Title)), Choices(TwitchSDK::ToFStringArray(x.Choices)), Duration(x.Duration), BitsVotingEnabled(x.BitsVotingEnabled), BitsPerVote(x.BitsPerVote), ChannelPointsVotingEnabled(x.ChannelPointsVotingEnabled), ChannelPointsPerVote(x.ChannelPointsPerVote) {}
    operator TwitchSDK::PollDefinition () const {
        TwitchSDK::PollDefinition v;
        v.Title = TwitchSDK::FromFString(Title);
        v.Choices = TwitchSDK::FromFStringArray(Choices);
        v.Duration = Duration;
        v.BitsVotingEnabled = BitsVotingEnabled;
        v.BitsPerVote = BitsPerVote;
        v.ChannelPointsVotingEnabled = ChannelPointsVotingEnabled;
        v.ChannelPointsPerVote = ChannelPointsPerVote;
        return v;
    }
    /** Question displayed for the poll. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") FString Title;
    /** Array of the poll choices. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") TArray<FString> Choices;
    /** Total duration for the poll (in seconds). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int64 Duration = 0;
    /** Indicates if Bits can be used for voting. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool BitsVotingEnabled = false;
    /** Number of Bits required to vote once with Bits. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 BitsPerVote = 0;
    /** Indicates if Channel Points can be used for voting. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") bool ChannelPointsVotingEnabled = false;
    /** Number of Channel Points required to vote once with Channel Points. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Twitch") int32 ChannelPointsPerVote = 0;
};

