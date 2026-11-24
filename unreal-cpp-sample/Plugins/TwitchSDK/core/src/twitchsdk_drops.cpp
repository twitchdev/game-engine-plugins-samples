#include "twitchsdk_impl.hpp"

// Twitch Drops "offline inventory" support.
//
// These calls use the logged-in user's access token, so Twitch scopes the results to that
// user: GET returns only the current user's entitlements, and PATCH may only fulfill the
// current user's entitlements. See https://dev.twitch.tv/docs/drops/technical-guide/
// (the "Twitch API (Offline Inventory)" flow).

using namespace TwitchSDK;

namespace {
	// Maps the Twitch API's fulfillment_status strings to the DropFulfillmentStatus enum.
	DropFulfillmentStatus ParseFulfillmentStatus(string_view s) {
		if (s == STR("CLAIMED"sv))
		{
			return DropFulfillmentStatus::Claimed;
		}
		if (s == STR("FULFILLED"sv))
		{
			return DropFulfillmentStatus::Fulfilled;
		}
		return DropFulfillmentStatus::Unknown;
	}

	// Maps the DropFulfillmentStatus enum to the Twitch API's fulfillment_status strings.
	// Returns an empty view for Unknown (used to mean "no filter").
	string_view FulfillmentStatusToString(DropFulfillmentStatus s) {
		switch (s)
		{
		case DropFulfillmentStatus::Claimed: return STR("CLAIMED"sv);
		case DropFulfillmentStatus::Fulfilled: return STR("FULFILLED"sv);
		default: return string_view();
		}
	}
}

void TwitchSDK::TwitchSDKApiImpl::GetDropEntitlements(const DropEntitlementQuery& req, ResolveFn<const DropEntitlementList&> resolve, RejectFn reject) {
	std::lock_guard<std::recursive_mutex> lock(Mutex);

	string url = STR("https://api.twitch.tv/helix/entitlements/drops?"s);
	if (((string_view)req.GameId).size())
	{
		url += STR("&game_id="sv);
		EncodeUriComponent(url, req.GameId);
	}
	auto statusStr = FulfillmentStatusToString(req.FulfillmentStatus);
	if (statusStr.size())
	{
		url += STR("&fulfillment_status="sv);
		url += statusStr;
	}

	TwitchSDKApiImpl::CallApi(
		shared_from_this(),
		std::make_shared<LoggedInApiCall>(url),
		[resolve, reject](const WebRequestResult& r) {
			DropEntitlementList ret;

			try
			{
				auto j = Json::Parse(r.ResponseBody);
				auto data = j.at("data");
				for (size_t i = 0; i < data.array_size(); i++)
				{
					auto ele = data.at(i);
					DropEntitlement entitlement;
					entitlement.Id = ele.at("id").to_string();
					entitlement.BenefitId = ele.at("benefit_id").to_string();
					entitlement.Timestamp = ele.at("timestamp").to_string();
					entitlement.UserId = ele.at("user_id").to_string();
					entitlement.GameId = ele.at("game_id").to_string();
					entitlement.FulfillmentStatus = ParseFulfillmentStatus(ele.at("fulfillment_status").to_string());
					entitlement.LastUpdated = ele.at("last_updated").to_string();
					ret.Entitlements.push_back(std::move(entitlement));
				}
			}
			catch (...)
			{
			}

			resolve(ret);
		},
		reject
	);
}

void TwitchSDK::TwitchSDKApiImpl::FulfillDropEntitlements(const FulfillDropEntitlementsRequest& req, ResolveFn<void> resolve, RejectFn reject) {
	std::lock_guard<std::recursive_mutex> lock(Mutex);

	if (req.EntitlementIds.size() == 0)
	{
		// Nothing to do; treat an empty request as a successful no-op rather than sending a
		// pointless request that Twitch would reject.
		resolve();
		return;
	}

	Json body;
	body.set_property("fulfillment_status", Json(STR("FULFILLED"sv)));
	std::vector<Json> ids;
	ids.reserve(req.EntitlementIds.size());
	for (auto& id : req.EntitlementIds)
	{
		ids.push_back(Json((string_view)id));
	}
	body.set_array("entitlement_ids", std::move(ids));

	TwitchSDKApiImpl::CallApi(
		shared_from_this(),
		std::make_shared<LoggedInApiCall>(STR("https://api.twitch.tv/helix/entitlements/drops"s), body.serialize(), HttpMethod::Patch),
		[resolve, reject](const WebRequestResult& r) {
			// The response contains a per-id status array; for this user-token flow we treat any
			// successful HTTP response as success. (CallApi already surfaced non-2xx as errors.)
			resolve();
		},
		reject
	);
}
