#include "pal.hpp"
#include "marshalling.hpp"

using namespace TwitchSDK;
using namespace TwitchSDK::PAL;
using namespace TwitchSDK::Marshalling;

void ProxyPAL::WebRequest(
    HttpMethod method,
    TwitchSDK::string_view uri,
    TwitchSDK::string_view contentType,
    TwitchSDK::string_view clientId,
    TwitchSDK::string_view authorization,
    TwitchSDK::string_view requestBody,
    std::function<void(const WebRequestResult&)> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedWebRequestRequest req(
            method,
            uri,
            contentType,
            clientId,
            authorization,
            requestBody,
            [cb](const ManagedWebRequestResult& r) { WebRequestResult res; res.HttpStatus = r.HttpStatus; res.ResponseBody = r.ResponseBody;  cb(res); },
            rej
        );
        Impl(Parent, 1126061481, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::Sleep(
    int32_t milliseconds,
    std::function<void()> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedSleepRequest req(
            milliseconds,
            [cb](const ManagedNone&) { cb(); },
            rej
        );
        Impl(Parent, -764117084, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::ReadFile(
    TwitchSDK::string_view path,
    std::function<void(TwitchSDK::string_view)> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedReadFileRequest req(
            path,
            [cb](const ManagedPlainString& r) { cb(r.Data); },
            rej
        );
        Impl(Parent, 1884918045, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::WriteFile(
    TwitchSDK::string_view path,
    TwitchSDK::string_view data,
    std::function<void()> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedWriteFileRequest req(
            path,
            data,
            [cb](const ManagedNone&) { cb(); },
            rej
        );
        Impl(Parent, 981515029, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::Log(
    LogLevel level,
    TwitchSDK::string_view message,
    std::function<void()> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedLogRequest req(
            level,
            message,
            [cb](const ManagedNone&) { cb(); },
            rej
        );
        Impl(Parent, -1432180053, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::CreateWebSocket(
    TwitchSDK::string_view url,
    TwitchSDK::string_view protocol,
    std::function<void(int32_t)> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedCreateWebSocketRequest req(
            url,
            protocol,
            [cb](const ManagedPlainInt& r) { cb(r.Data); },
            rej
        );
        Impl(Parent, -2108203796, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::SendWebSocketMessage(
    int32_t handle,
    TwitchSDK::string_view message,
    std::function<void()> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedSendWebSocketMessageRequest req(
            handle,
            message,
            [cb](const ManagedNone&) { cb(); },
            rej
        );
        Impl(Parent, -1797785895, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::RecvWebSocketMessage(
    int32_t handle,
    int32_t timeoutSeconds,
    std::function<void(TwitchSDK::string_view)> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedRecvWebSocketMessageRequest req(
            handle,
            timeoutSeconds,
            [cb](const ManagedPlainString& r) { cb(r.Data); },
            rej
        );
        Impl(Parent, 1978333561, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

void ProxyPAL::CloseWebSocket(
    int32_t handle,
    std::function<void()> cb,
    std::function<void(const std::exception&)> rej)
{
    try {
        if (!Parent) throw MissingPalImpl("Has disposed!");

        ManagedCloseWebSocketRequest req(
            handle,
            [cb](const ManagedNone&) { cb(); },
            rej
        );
        Impl(Parent, 491657886, &req);
    }
    catch (const std::exception& e) {
        rej(e);
    }
}

