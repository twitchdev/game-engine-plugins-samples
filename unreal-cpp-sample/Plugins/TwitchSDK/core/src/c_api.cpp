#include <cassert>
#include "c_api.hpp"

using namespace TwitchSDK;
using namespace TwitchSDK::PAL;
using namespace TwitchSDK::Marshalling;

std::shared_ptr<ProxyPAL>* ProxyPAL_new(void* parent, ProxyPALImpl impl)
{
	return new std::shared_ptr<ProxyPAL>(std::make_shared<ProxyPAL>(parent, impl));
}

void ProxyPAL_Dispose(std::shared_ptr<ProxyPAL>* pal)
{
	pal->get()->Dispose();
	delete pal;
}

std::shared_ptr<TwitchSDKApi>* TwitchSDKApi_new(
	std::shared_ptr<PlatformAbstractionLayer>* pal,
	const char16_t* clientId,
	int32_t useEventSubProxy)
{
	return new std::shared_ptr<TwitchSDKApi>(TwitchSDKApi::Initialize(*pal, clientId, useEventSubProxy != 0));
}

void TwitchSDKApi_Dispose(std::shared_ptr<TwitchSDKApi>* api)
{
	delete api;
}

const char16_t* TwitchSDK_GetVersion()
{
	return TwitchSDK::Version.c_str();
}
