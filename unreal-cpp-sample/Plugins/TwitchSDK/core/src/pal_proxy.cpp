#include "pal.hpp"
#include "marshalling.hpp"

using namespace TwitchSDK;
using namespace TwitchSDK::PAL;
using namespace TwitchSDK::Marshalling;

ProxyPAL::ProxyPAL(void* parent, ProxyPALImpl impl)
	: Parent(parent)
	, Impl(impl)
{
}

void ProxyPAL::Dispose()
{
	Parent = nullptr;
}
