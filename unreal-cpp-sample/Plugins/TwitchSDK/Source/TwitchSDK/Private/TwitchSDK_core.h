#pragma once

// Private prelude for the amalgamated core translation unit (TwitchSDK_core.cpp).
// Pulls in the public module header so that the core string/JSON configuration macros
// (TWITCHSDK_STR_*, TWITCHSDK_JSON_EXTERNAL) and the FString<->core-string helpers are
// established before any core .cpp files are included below it.
#include "TwitchSDK.h"
