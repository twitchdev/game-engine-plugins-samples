#pragma once

#include "Modules/ModuleManager.h"
#include <HAL/Platform.h>

// Unreal's TCHAR is defined as either wchar_t or char16_t depending on the underlying platform.
// We configure the core library accordingly, so that FString matches TwitchSDK::string.
#if PLATFORM_TCHAR_IS_CHAR16
#define TWITCHSDK_STR_U16STRING
#else
#define TWITCHSDK_STR_WSTRING
#endif

// We have our own JSON implementation in TwitchSDK_core.cpp
#define TWITCHSDK_JSON_EXTERNAL
#include "twitchsdk.hpp"

#include "TwitchSDK.generated.h"



UENUM(BlueprintType)
enum class FTwitchSDKOAuthScope : uint8 {
    ChannelManagePolls UMETA(DisplayName = "channel:manage:polls"),
    ChannelManagePredictions UMETA(DisplayName = "channel:manage:predictions"),
    ChannelManageBroadcast UMETA(DisplayName = "channel:manage:broadcast"),
    ChannelManageRedemptions UMETA(DisplayName = "channel:manage:redemptions"),
    ChannelReadHype_Train UMETA(DisplayName = "channel:read:hype_train"),
    ClipsEdit UMETA(DisplayName = "clips:edit"),
    UserReadSubscriptions UMETA(DisplayName = "user:read:subscriptions"),
    BitsRead UMETA(DisplayName = "bits:read"),
};


namespace TwitchSDK {
    inline FString ToFString(TwitchSDK::string_view s) {
        return FString(s.size(), s.data());
    }
    inline TwitchSDK::string_view FromFString(const FString& fs) {
        return TwitchSDK::string_view(*fs, fs.Len());
    }
    inline std::vector<TwitchSDK::string_holder> FromFStringArray(const TArray<FString>& a) {
        std::vector<TwitchSDK::string_holder> v;
        v.reserve(a.Num());
        for (auto& s : a)
        {
            v.push_back(FromFString(s));
        }
        return v;
    }
    inline TArray<FString> ToFStringArray(const std::vector<TwitchSDK::string_holder>& a) {
        TArray<FString> v;
        v.Reserve(a.size());
        for (auto& s : a)
        {
            v.Add(ToFString(s));
        }
        return v;
    }
    template<class T, class U> TArray<U> ToUnrealArray(const std::vector<T>& a) {
        TArray<U> v;
        for (auto& x : a)
        {
            v.Emplace(x);
        }
        return v;
    }
    template<class T, class U> std::vector<T> FromUnrealArray(const TArray<U>& a) {
        std::vector<T> v;
        for (auto& x : a)
        {
            v.emplace_back(x);
        }
        return v;
    }


    // Helper functions

    // Exported so game modules can call it (without TWITCHSDK_API it fails to link outside the plugin).
    TWITCHSDK_API TwitchSDK::string BuildOAuthScopes(TArray<FTwitchSDKOAuthScope> scopes, TArray<FString> customScopes = {});
}


class UnrealPAL;
class FTwitchSDKModule : public IModuleInterface
{
public:
	std::shared_ptr<TwitchSDK::TwitchSDKApi> Core;
	std::shared_ptr<UnrealPAL> PAL;
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** singleton for the module while loaded and available */
	static FTwitchSDKModule* Singleton;

	static FTwitchSDKModule& Get() {
		check(Singleton);
		return *Singleton;
	}
};

DECLARE_LOG_CATEGORY_EXTERN(LogTwitchSDK, Log, All);

