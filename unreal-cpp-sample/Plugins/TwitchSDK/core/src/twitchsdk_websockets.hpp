#pragma once

#include <memory>
#include "pal.hpp"

namespace TwitchSDK::PAL {
	class Websocket : public std::enable_shared_from_this<Websocket> {
		std::weak_ptr<PlatformAbstractionLayer> PAL;
		int32_t Handle;

	public:
		Websocket(
			std::weak_ptr<PlatformAbstractionLayer> pal,
			int32_t handle
		) : PAL(pal), Handle(handle) {
		}

		~Websocket() {
			auto pal = PAL.lock();
			if (!pal) return;

			pal->CloseWebSocket(Handle, []() {}, [](const std::exception&) {});
		}

		static void Open(
			std::shared_ptr<PlatformAbstractionLayer> pal,
			TwitchSDK::string_view url,
			TwitchSDK::string_view protocol,
			std::function<void(std::shared_ptr<Websocket>)> cb,
			std::function<void(const std::exception&)> rej
		) {
			pal->CreateWebSocket(url, protocol,
				[cb, rej, pal](int32_t handle)
				{
					try {
						cb(std::make_shared<Websocket>(pal, handle));
					}
					catch (const std::exception& e) {
						rej(e);
					}
				},
				rej
			);
		}

		/*
		 * Lifecycle: Send and Recv always have a shared_ptr to this Websocket instance in their
		 * respective callbacks, so that the Websocket can never get closed while a Send/Recv is still active.
		 */
		void Recv(
			int32_t timeoutSeconds,
			std::function<void(TwitchSDK::string_view)> cb,
			std::function<void(const std::exception&)> rej
		) {
			auto pal = PAL.lock();
			if (!pal) {
				rej(std::runtime_error("PAL is gone"));
				return;
			}

			auto cb2 = [cb, self = shared_from_this()](TwitchSDK::string_view s) { cb(s); };
			pal->RecvWebSocketMessage(Handle, timeoutSeconds, cb2, rej);
		}

		void Send(TwitchSDK::string_view message) {
			auto pal = PAL.lock();
			if (!pal) {
				throw std::runtime_error("PAL is gone");
			}

			pal->SendWebSocketMessage(Handle, message, [self = shared_from_this()]() {}, [](const std::exception&) {});
		}
	};
}
