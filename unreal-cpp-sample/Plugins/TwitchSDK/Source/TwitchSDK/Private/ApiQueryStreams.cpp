#include "ApiQueryStreams.h"

void UApiQueryStreams::Activate() {
    TWeakObjectPtr<UApiQueryStreams> weak(this);
    auto exception_handler = [weak](const std::exception& e) {
        if (weak.IsValid() && weak->Error.IsBound())
            weak->Error.Broadcast(FTwitchSDKStreamQueryResult(), FString(e.what()));
        else
            UE_LOG(LogTwitchSDK, Error, TEXT("QueryStreams error: %s"), UTF8_TO_TCHAR(e.what()));
    };
    TwitchSDK::StreamQuery v;
    v.After = TwitchSDK::FromFString(After);
    v.First = First;
    v.GameIds = TwitchSDK::FromFStringArray(GameIds);
    v.Languages = TwitchSDK::FromFStringArray(Languages);
    v.UserIds = TwitchSDK::FromFStringArray(UserIds);
    v.UserLogins = TwitchSDK::FromFStringArray(UserLogins);
    try {
        FTwitchSDKModule::Get().Core->QueryStreams(
            v,
            [weak](const TwitchSDK::StreamQueryResult& r) {
                if (!weak.IsValid()) return;
                FTwitchSDKStreamQueryResult v(r);
                weak->Done.Broadcast(v, FString());
            },
            exception_handler
        );
    } catch (const std::exception& e) {
        exception_handler(e);
    }
}

UApiQueryStreams* UApiQueryStreams::QueryStreams(FString after, int32 first, TArray<FString> gameIds, TArray<FString> languages, TArray<FString> userIds, TArray<FString> userLogins) {
    auto ptr = NewObject<UApiQueryStreams>();
    ptr->After = after;
    ptr->First = first;
    ptr->GameIds = gameIds;
    ptr->Languages = languages;
    ptr->UserIds = userIds;
    ptr->UserLogins = userLogins;
    return ptr;
}
