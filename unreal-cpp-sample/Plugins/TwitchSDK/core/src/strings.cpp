// We silence this warning because this functionality works fine and there is no reasonable
// replacement right now. The only alternative is to reimplement wstring_convert.
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING 1

#include <codecvt>
#include <locale>
#include "twitchsdk_json.hpp"


// Work around STL bug: covdecvt has a protected (???) destructor according to the standard
// https://stackoverflow.com/a/41744847
template<class I, class E, class S>
struct codecvt : std::codecvt<I, E, S>
{
    ~codecvt()
    { }
};



TwitchSDK::string_holder TwitchSDK::from_utf8(std::string_view s)
{
#if defined(TWITCHSDK_STR_WSTRING) || defined(TWITCHSDK_STR_U16STRING)
	std::wstring_convert<codecvt<twitchsdkchar, char, std::mbstate_t>, twitchsdkchar> convert;
	return convert.from_bytes(s.data(), s.data() + s.size());
#else
	return s;
#endif
}

TwitchSDK::stdstring_holder TwitchSDK::to_utf8(string_view s)
{
#if defined(TWITCHSDK_STR_WSTRING) || defined(TWITCHSDK_STR_U16STRING)
	std::wstring_convert<codecvt<twitchsdkchar, char, std::mbstate_t>, twitchsdkchar> convert;
	return convert.to_bytes(s.data(), s.data() + s.size());
#else
	return s;
#endif
}
