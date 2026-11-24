#include "ApiFulfillDropEntitlements.h"

void UApiFulfillDropEntitlements::Activate() {
    TWeakObjectPtr<UApiFulfillDropEntitlements> weak(this);
    auto exception_handler = [weak](const std::exception& e) {
        if (weak.IsValid() && weak->Error.IsBound())
            weak->Error.Broadcast(FString(e.what()));
        else
            UE_LOG(LogTwitchSDK, Error, TEXT("FulfillDropEntitlements error: %s"), UTF8_TO_TCHAR(e.what()));
    };
    auto v = Req;
    try {
        FTwitchSDKModule::Get().Core->FulfillDropEntitlements(
            v,
            [weak]() { if (weak.IsValid()) weak->Done.Broadcast(FString()); },
            exception_handler
        );
    } catch (const std::exception& e) {
        exception_handler(e);
    }
}

UApiFulfillDropEntitlements* UApiFulfillDropEntitlements::FulfillDropEntitlements(FTwitchSDKFulfillDropEntitlementsRequest req) {
    auto ptr = NewObject<UApiFulfillDropEntitlements>();
    ptr->Req = req;
    return ptr;
}
