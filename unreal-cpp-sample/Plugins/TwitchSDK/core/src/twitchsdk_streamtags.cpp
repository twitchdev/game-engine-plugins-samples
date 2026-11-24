#include "twitchsdk_impl.hpp"

void TwitchSDK::TwitchSDKApiImpl::AddStreamTag(TwitchSDK::string_view id, ResolveFn<void> resolve, RejectFn reject) {
	std::lock_guard<std::recursive_mutex> lock(Mutex);
	TagOperationQueue.push_back(TagOperation(string(id), true, std::move(resolve), std::move(reject)));
	TagsApplyUpdates(shared_from_this());
}

void TwitchSDK::TwitchSDKApiImpl::RemoveStreamTag(TwitchSDK::string_view id, ResolveFn<void> resolve, RejectFn reject) {
	std::lock_guard<std::recursive_mutex> lock(Mutex);
	TagOperationQueue.push_back(TagOperation(string(id), false, std::move(resolve), std::move(reject)));
	TagsApplyUpdates(shared_from_this());
}

void TwitchSDK::TwitchSDKApiImpl::TagsApplyUpdates(Self self) {
	std::lock_guard<std::recursive_mutex> lock(self->Mutex);
	if (self->TagsUpdateInProgress)
	{
		return;
	}

	// assume that this function only gets called after adding someting to TagOperationQueue
	// and hence checking for an empty TagOperationQueue would be redundant here

	self->TagsUpdateInProgress = true;

	self->MyUserInfo.Await(
		[self](const UserInfo& userinfo) {
			std::lock_guard<std::recursive_mutex> lock(self->Mutex);
			string uri = STR("https://api.twitch.tv/helix/streams/tags?broadcaster_id="s);
			uri += userinfo.ChannelId;
			CallApi(self,
				std::make_shared<LoggedInApiCall>(uri),
				[self, uri](const WebRequestResult& wrr) {
					std::lock_guard<std::recursive_mutex> lock(self->Mutex);

					std::list<string> existingTags;
					auto j = Json::Parse(wrr.ResponseBody);
					auto data = j.at("data");
					auto arraylen = data.array_size();
					for (size_t i = 0; i < arraylen; i++)
					{
						auto ele = data.at(i);
						if (ele.at("is_auto").to_bool())
						{
							continue;
						}
						existingTags.push_back(ele.at("tag_id").to_string());
					}

					// we swap it out so that subsequent operations that arrive while our HTTP request is underway
					// do not get cleared out erroneously
					std::list<TagOperation> queue;
					std::swap(queue, self->TagOperationQueue);
					bool changedSomething = false;
					for (auto& op : queue)
					{
						auto existingElement = std::find(existingTags.begin(), existingTags.end(), op.Tag);
						bool existsAlready = existingElement != existingTags.end();

						if (op.Add == existsAlready)
						{
							// nothing to do, already in the desired state
							continue;
						}

						if (op.Add)
						{
							existingTags.push_back(op.Tag);
						}
						else
						{
							existingTags.erase(existingElement);
						}
						changedSomething = true;
					}

					if (!changedSomething)
					{
						// nothing to do, bail out
						self->TagsUpdateInProgress = false;
						for (auto& op : queue)
						{
							op.Resolve();
						}
						return;
					}

					std::vector<Json> tagids;
					for (auto& tag : existingTags)
					{
						tagids.push_back(Json(tag));
					}
					Json req;
					req.set_array("tag_ids", std::move(tagids));

					TwitchSDKApiImpl::CallApi(
						self,
						std::make_shared<LoggedInApiCall>(uri, req.serialize(), HttpMethod::Put),
						[self, queue](const WebRequestResult& r) {
							self->TagsUpdateInProgress = false;
							for (auto& op : queue)
							{
								op.Resolve();
							}
						},
						[self, queue](const std::exception& e) {
							std::lock_guard<std::recursive_mutex> lock(self->Mutex);
							self->TagsUpdateInProgress = false;
							for (auto& op : queue)
							{
								op.Reject(e);
							}
						}
					);
				},
				[self](const std::exception& e) {
					std::lock_guard<std::recursive_mutex> lock(self->Mutex);
					self->TagsUpdateInProgress = false;
					Log(self, LogLevel::Debug, "Failed to initiate tag update: %s", e.what());
				}
			);
		}
	);
}
