#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "TwitchSDKStructs.h"

#include "ApiGetDropEntitlements.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncDoneApiGetDropEntitlements, FTwitchSDKDropEntitlementList, Result, FString, Error);

UCLASS(meta=(HideThen=true))
class TWITCHSDK_API UApiGetDropEntitlements : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

    FString GameId;
    FTwitchSDKDropFulfillmentStatus FulfillmentStatus;

public:
    UPROPERTY(BlueprintAssignable)
    FAsyncDoneApiGetDropEntitlements Done;
    UPROPERTY(BlueprintAssignable)
    FAsyncDoneApiGetDropEntitlements Error;

    virtual void Activate() override;

    /**
     * Gets the Twitch Drop entitlements for the currently logged-in user.
     *
     * This supports the "offline inventory" Drops model: a game grants a user's own Drop rewards by polling for the user's claimed entitlements and marking them fulfilled once granted in-game. Because the plugin uses the logged-in user's access token, this returns entitlements for that user only. The entitlement's game category must be owned by a developer organization that the owner of your Client-Id belongs to.
     *
     * There is no real-time push for Drop grants in this model; call this periodically or when the user requests a Drops check (e.g. a "Check for Drops" button).
     *
     * @param gameId Optional. Restrict results to entitlements for a specific game (category) ID. Leave empty to return entitlements across all of your games.
     * @param fulfillmentStatus Optional. Restrict results to entitlements with a specific fulfillment status (typically DropFulfillmentStatus::Claimed to find rewards that still need granting). Leave as DropFulfillmentStatus::Unknown to return entitlements of any status.
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Twitch")
    static UApiGetDropEntitlements* GetDropEntitlements(FString gameId, FTwitchSDKDropFulfillmentStatus fulfillmentStatus);
};
