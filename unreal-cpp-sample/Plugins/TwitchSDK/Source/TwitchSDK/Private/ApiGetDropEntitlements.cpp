#include "ApiGetDropEntitlements.h"

void UApiGetDropEntitlements::Activate() {
    TWeakObjectPtr<UApiGetDropEntitlements> weak(this);
    auto exception_handler = [weak](const std::exception& e) {
        if (weak.IsValid() && weak->Error.IsBound())
            weak->Error.Broadcast(FTwitchSDKDropEntitlementList(), FString(e.what()));
        else
            UE_LOG(LogTwitchSDK, Error, TEXT("GetDropEntitlements error: %s"), UTF8_TO_TCHAR(e.what()));
    };
    TwitchSDK::DropEntitlementQuery v;
    v.GameId = TwitchSDK::FromFString(GameId);
    v.FulfillmentStatus = TwitchSDK::DropFulfillmentStatus(FulfillmentStatus);
    try {
        FTwitchSDKModule::Get().Core->GetDropEntitlements(
            v,
            [weak](const TwitchSDK::DropEntitlementList& r) {
                if (!weak.IsValid()) return;
                FTwitchSDKDropEntitlementList v(r);
                weak->Done.Broadcast(v, FString());
            },
            exception_handler
        );
    } catch (const std::exception& e) {
        exception_handler(e);
    }
}

UApiGetDropEntitlements* UApiGetDropEntitlements::GetDropEntitlements(FString gameId, FTwitchSDKDropFulfillmentStatus fulfillmentStatus) {
    auto ptr = NewObject<UApiGetDropEntitlements>();
    ptr->GameId = gameId;
    ptr->FulfillmentStatus = fulfillmentStatus;
    return ptr;
}
