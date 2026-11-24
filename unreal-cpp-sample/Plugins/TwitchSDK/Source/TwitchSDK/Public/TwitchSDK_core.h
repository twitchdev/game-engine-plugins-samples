#pragma once
#include "TwitchSDK.h"

inline FString ToFString(TwitchSDK::string_view s) {
	return FString(s.size(), s.data());
}
inline TwitchSDK::string_view FromFString(const FString& fs) {
	return TwitchSDK::string_view(*fs, fs.Len());
}
