#pragma once

#include "CoreMinimal.h"

#include "UI/TwitchSampleContext.h"

/**
 * Keeps one EventSub stream open and hands each event to a callback until stopped.
 *
 * Uses the raw API (SubscribeToEventStream, WaitFor...Event, CloseEventStream) so errors are
 * reported. Everything runs on the game thread. A generation counter makes a stopped (or
 * restarted) subscription ignore results that arrive late.
 */
template <class TEvent>
class TSampleEventSubscription : public TSharedFromThis<TSampleEventSubscription<TEvent>>
{
public:
	using FWaitFn = void (TwitchSDK::ITwitchSDKApiCommon::*)(const TwitchSDK::EventStreamDesc&, TwitchSDK::ResolveFn<const TEvent&>, TwitchSDK::RejectFn);

	enum class EState : uint8
	{
		Off,
		Starting,
		Listening,
	};

	TSampleEventSubscription(FString InName, TwitchSDK::EventStreamKind InKind, FWaitFn InWaitFn, TFunction<void(const TEvent&)> InOnEvent)
		: Name(MoveTemp(InName)), Kind(InKind), WaitFn(InWaitFn), OnEvent(MoveTemp(InOnEvent))
	{
	}

	EState GetState() const { return State; }
	bool IsOn() const { return State != EState::Off; }
	const FString& GetName() const { return Name; }

	void Start(const TSharedRef<FTwitchSampleContext>& Context)
	{
		if (State != EState::Off)
			return;

		const int32 MyGeneration = ++Generation;
		State = EState::Starting;
		WeakContext = Context;

		TwitchSDK::EventStreamRequest Request;
		Request.Kind = Kind;

		const TWeakPtr<TSampleEventSubscription> WeakThis = this->AsShared();
		Context->template Call<TwitchSDK::EventStreamDesc>(
			Name,
			[Request](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.SubscribeToEventStream(Request, Resolve, Reject); },
			[WeakThis, MyGeneration](const TwitchSDK::EventStreamDesc& Desc)
			{
				const TSharedPtr<TSampleEventSubscription> This = WeakThis.Pin();
				if (!This)
					return;
				if (MyGeneration != This->Generation)
				{
					This->Close(Desc); // stopped while subscribing
					return;
				}
				This->Desc = Desc;
				This->State = EState::Listening;
				if (const TSharedPtr<FTwitchSampleContext> Ctx = This->WeakContext.Pin())
					Ctx->Success(FString::Printf(TEXT("Listening for %s."), *This->Name));
				This->WaitForNext(MyGeneration);
			},
			[WeakThis, MyGeneration](const FString& Message) { if (const auto This = WeakThis.Pin()) This->Fail(MyGeneration, Message); },
			/*bTrackAsRunning*/ false);
	}

	void Stop(bool bLog = true)
	{
		if (State == EState::Off)
			return;

		++Generation;
		const bool bWasListening = State == EState::Listening;
		State = EState::Off;
		if (bWasListening)
			Close(Desc);

		if (bLog)
		{
			if (const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin())
				Ctx->Info(FString::Printf(TEXT("Stopped listening for %s."), *Name));
		}
	}

private:
	void WaitForNext(int32 MyGeneration)
	{
		const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin();
		if (!Ctx || MyGeneration != Generation)
			return;

		const TWeakPtr<TSampleEventSubscription> WeakThis = this->AsShared();
		const TwitchSDK::EventStreamDesc CurrentDesc = Desc;
		const FWaitFn Wait = WaitFn;
		Ctx->template Call<TEvent>(
			Name,
			[CurrentDesc, Wait](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { (Api.*Wait)(CurrentDesc, Resolve, Reject); },
			[WeakThis, MyGeneration](const TEvent& Event)
			{
				const TSharedPtr<TSampleEventSubscription> This = WeakThis.Pin();
				if (!This || MyGeneration != This->Generation)
					return;
				This->OnEvent(Event);
				This->WaitForNext(MyGeneration);
			},
			[WeakThis, MyGeneration](const FString& Message) { if (const auto This = WeakThis.Pin()) This->Fail(MyGeneration, Message); },
			/*bTrackAsRunning*/ false);
	}

	void Fail(int32 MyGeneration, const FString& Message)
	{
		if (MyGeneration != Generation)
			return; // already stopped; closing the stream rejects the pending wait
		State = EState::Off;
		if (const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin())
			Ctx->Error(FString::Printf(TEXT("%s: %s"), *Name, *Message));
	}

	void Close(const TwitchSDK::EventStreamDesc& StreamDesc)
	{
		if (const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin())
		{
			Ctx->CallVoid(
				Name,
				[StreamDesc](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.CloseEventStream(StreamDesc, Resolve, Reject); },
				nullptr,
				[](const FString&) {}, // closing is best effort
				/*bTrackAsRunning*/ false);
		}
	}

	FString Name;
	TwitchSDK::EventStreamKind Kind;
	FWaitFn WaitFn;
	TFunction<void(const TEvent&)> OnEvent;

	TWeakPtr<FTwitchSampleContext> WeakContext;
	TwitchSDK::EventStreamDesc Desc;
	EState State = EState::Off;
	int32 Generation = 0;
};
