#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "TwitchSDKStructs.h"

#include "ApiFulfillDropEntitlements.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAsyncDoneApiFulfillDropEntitlements, FString, Error);

UCLASS(meta=(HideThen=true))
class TWITCHSDK_API UApiFulfillDropEntitlements : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

    FTwitchSDKFulfillDropEntitlementsRequest Req;

public:
    UPROPERTY(BlueprintAssignable)
    FAsyncDoneApiFulfillDropEntitlements Done;
    UPROPERTY(BlueprintAssignable)
    FAsyncDoneApiFulfillDropEntitlements Error;

    virtual void Activate() override;

    /**
     * Marks one or more of the logged-in user's Drop entitlements as fulfilled.
     *
     * Call this after your game has successfully granted the reward(s) for the given entitlements in-game. Because the plugin uses the logged-in user's access token, you may only fulfill entitlements that belong to that user.
     *
     */
    UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Twitch")
    static UApiFulfillDropEntitlements* FulfillDropEntitlements(FTwitchSDKFulfillDropEntitlementsRequest req);
};
