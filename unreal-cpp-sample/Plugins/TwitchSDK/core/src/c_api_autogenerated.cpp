#include <stdexcept>
#include "c_api.hpp"

using namespace TwitchSDK;
using namespace TwitchSDK::PAL;
using namespace TwitchSDK::Marshalling;


void TwitchSDKApi_GetAuthState(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedNone* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        api->get()->GetAuthState(
            [cb, pl](const AuthState& r) {
                ManagedAuthState managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetAuthenticationInfo(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->GetAuthenticationInfo(
            v,
            [cb, pl](const AuthenticationInfo& r) {
                ManagedAuthenticationInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_LogOut(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedNone* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        api->get()->LogOut(
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetMyUserInfo(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedNone* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        api->get()->GetMyUserInfo(
            [cb, pl](const UserInfo& r) {
                ManagedUserInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetUserInfoById(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->GetUserInfoById(
            v,
            [cb, pl](const UserInfo& r) {
                ManagedUserInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetUserInfoByLoginName(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->GetUserInfoByLoginName(
            v,
            [cb, pl](const UserInfo& r) {
                ManagedUserInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetMyStreamInfo(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedNone* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        api->get()->GetMyStreamInfo(
            [cb, pl](const StreamInfo& r) {
                ManagedStreamInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_QueryStreams(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedStreamQuery* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        StreamQuery v(*p);
        api->get()->QueryStreams(
            v,
            [cb, pl](const StreamQueryResult& r) {
                ManagedStreamQueryResult managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_CheckUserSubscription(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->CheckUserSubscription(
            v,
            [cb, pl](const UserSubscriptionCheckResult& r) {
                ManagedUserSubscriptionCheckResult managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_CreateClip(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainBool* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->CreateClip(
            v,
            [cb, pl](const ClipInfo& r) {
                ManagedClipInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_CreateStreamMarker(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->CreateStreamMarker(
            v,
            [cb, pl](const StreamMarkerInfo& r) {
                ManagedStreamMarkerInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_CreatePoll(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPollDefinition* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        PollDefinition v(*p);
        api->get()->CreatePoll(
            v,
            [cb, pl](const PollInfo& r) {
                ManagedPollInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForPollUpdate(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->WaitForPollUpdate(
            v,
            [cb, pl](const PollInfo& r) {
                ManagedPollInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_EndPoll(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEndPollRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EndPollRequest v(*p);
        api->get()->EndPoll(
            v,
            [cb, pl](const PollInfo& r) {
                ManagedPollInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_UnsubscribeFromPoll(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->UnsubscribeFromPoll(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_CreatePrediction(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPredictionDefinition* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        PredictionDefinition v(*p);
        api->get()->CreatePrediction(
            v,
            [cb, pl](const PredictionInfo& r) {
                ManagedPredictionInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForPredictionUpdate(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->WaitForPredictionUpdate(
            v,
            [cb, pl](const PredictionInfo& r) {
                ManagedPredictionInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_EndPrediction(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEndPredictionRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EndPredictionRequest v(*p);
        api->get()->EndPrediction(
            v,
            [cb, pl](const PredictionInfo& r) {
                ManagedPredictionInfo managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_UnsubscribeFromPrediction(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedPlainString* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        auto v = p->Data;
        api->get()->UnsubscribeFromPrediction(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_ModifyChannelInformation(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedModifyChannelInfoRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        ModifyChannelInfoRequest v(*p);
        api->get()->ModifyChannelInformation(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetBitsLeaderboard(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedBitsLeaderboardRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        BitsLeaderboardRequest v(*p);
        api->get()->GetBitsLeaderboard(
            v,
            [cb, pl](const BitsLeaderboard& r) {
                ManagedBitsLeaderboard managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_GetDropEntitlements(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedDropEntitlementQuery* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        DropEntitlementQuery v(*p);
        api->get()->GetDropEntitlements(
            v,
            [cb, pl](const DropEntitlementList& r) {
                ManagedDropEntitlementList managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_FulfillDropEntitlements(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedFulfillDropEntitlementsRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        FulfillDropEntitlementsRequest v(*p);
        api->get()->FulfillDropEntitlements(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_ReplaceCustomRewards(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedCustomRewardList* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        CustomRewardList v(*p);
        api->get()->ReplaceCustomRewards(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_ResolveCustomReward(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedCustomRewardResolveRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        CustomRewardResolveRequest v(*p);
        api->get()->ResolveCustomReward(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_SubscribeToEventStream(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamRequest* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamRequest v(*p);
        api->get()->SubscribeToEventStream(
            v,
            [cb, pl](const EventStreamDesc& r) {
                ManagedEventStreamDesc managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForChannelSubscribeEvent(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->WaitForChannelSubscribeEvent(
            v,
            [cb, pl](const ChannelSubscribeEvent& r) {
                ManagedChannelSubscribeEvent managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForChannelFollowEvent(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->WaitForChannelFollowEvent(
            v,
            [cb, pl](const ChannelFollowEvent& r) {
                ManagedChannelFollowEvent managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForChannelCheerEvent(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->WaitForChannelCheerEvent(
            v,
            [cb, pl](const ChannelCheerEvent& r) {
                ManagedChannelCheerEvent managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForCustomRewardEvent(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->WaitForCustomRewardEvent(
            v,
            [cb, pl](const CustomRewardEvent& r) {
                ManagedCustomRewardEvent managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForHypeTrainEvent(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->WaitForHypeTrainEvent(
            v,
            [cb, pl](const HypeTrainEvent& r) {
                ManagedHypeTrainEvent managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_WaitForChannelRaidEvent(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->WaitForChannelRaidEvent(
            v,
            [cb, pl](const ChannelRaidEvent& r) {
                ManagedChannelRaidEvent managed(r);
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_CloseEventStream(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedEventStreamDesc* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        EventStreamDesc v(*p);
        api->get()->CloseEventStream(
            v,
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
void TwitchSDKApi_PrepareShutdown(std::shared_ptr<TwitchSDK::TwitchSDKApi>* api, const TwitchSDK::Marshalling::ManagedNone* p, TwitchSDK::Marshalling::TaskCallback cb, void* pl) {
    RejectCallback rej(cb, pl);
    try {
        if (!p->IsValid()) throw std::runtime_error("wrong parameter type");
        api->get()->PrepareShutdown(
            [cb, pl]() {
                ManagedNone managed;
                cb(&managed, pl);
            },
            rej
        );
    }
    catch (const std::exception& e) {
        rej(e);
    }
}
