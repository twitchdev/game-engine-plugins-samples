// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "TwitchSDK.h"
#include <pal.hpp>

#include <memory>

/**
 *
 */
class WebsocketHolder;
class UnrealPAL : public TwitchSDK::PAL::PlatformAbstractionLayer, public std::enable_shared_from_this<UnrealPAL>
{
    bool ShuttingDown;

    FCriticalSection WebsocketsLock;
    int NextWebsocketHandle;
    TMap<int, std::shared_ptr<WebsocketHolder>> Websockets;
public:
	UnrealPAL();

    void Shutdown();

    virtual void WebRequest(
        TwitchSDK::HttpMethod method,
        TwitchSDK::string_view uri,
        TwitchSDK::string_view contentType,
        TwitchSDK::string_view clientId,
        TwitchSDK::string_view authorization,
        TwitchSDK::string_view requestBody,
        std::function<void(const TwitchSDK::WebRequestResult&)> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void Sleep(
        int32_t milliseconds,
        std::function<void()> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void ReadFile(
        TwitchSDK::string_view path,
        std::function<void(TwitchSDK::string_view)> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void WriteFile(
        TwitchSDK::string_view path,
        TwitchSDK::string_view data,
        std::function<void()> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void Log(
        TwitchSDK::LogLevel level,
        TwitchSDK::string_view message,
        std::function<void()> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void CreateWebSocket(
        TwitchSDK::string_view url,
        TwitchSDK::string_view protocol,
        std::function<void(int32_t)> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void SendWebSocketMessage(
        int32_t handle,
        TwitchSDK::string_view message,
        std::function<void()> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void RecvWebSocketMessage(
        int32_t handle,
        int32_t timeoutSeconds,
        std::function<void(TwitchSDK::string_view)> cb,
        std::function<void(const std::exception&)> rej
    ) override;
    virtual void CloseWebSocket(
        int32_t handle,
        std::function<void()> cb,
        std::function<void(const std::exception&)> rej
    ) override;
};
