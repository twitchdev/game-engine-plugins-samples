#include "UI/Pages/SPredictionsPage.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#include "UI/SampleWidgets.h"
#include "UI/SStringListEditor.h"

using TwitchSDK::ToFString;
using TwitchSDK::FromFString;

namespace
{
	// Limits enforced by the Twitch Create Prediction endpoint.
	constexpr int32 MaxTitleLength = 45;
	constexpr int32 MaxOutcomeLength = 25;
	constexpr int32 MinOutcomes = 2;
	constexpr int32 MaxOutcomes = 10;
	constexpr int32 MinDuration = 30;
	constexpr int32 MaxDuration = 1800;

	FString StatusName(TwitchSDK::PredictionStatus Status)
	{
		switch (Status)
		{
		case TwitchSDK::PredictionStatus::Active: return TEXT("Active (taking bets)");
		case TwitchSDK::PredictionStatus::Locked: return TEXT("Locked (pick the winner)");
		case TwitchSDK::PredictionStatus::Resolved: return TEXT("Resolved");
		default: return TEXT("Canceled (points refunded)");
		}
	}

	bool IsOpen(TwitchSDK::PredictionStatus Status)
	{
		return Status == TwitchSDK::PredictionStatus::Active || Status == TwitchSDK::PredictionStatus::Locked;
	}
}

void SPredictionsPage::Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext)
{
	Context = InContext;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeConnectionHint() ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("Create a prediction"),
				TEXT("Viewers bet channel points on an outcome. You need to be a Twitch Affiliate or Partner, and Twitch allows one active prediction at a time."))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Title"), &Title, FString::Printf(TEXT("Up to %d characters."), MaxTitleLength)) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SStringListEditor)
			.Items(&Outcomes)
			.ItemLabel(TEXT("Outcome"))
			.MinItems(MinOutcomes)
			.MaxItems(MaxOutcomes)
			.Hint(FString::Printf(TEXT("%d to %d outcomes, up to %d characters each."), MinOutcomes, MaxOutcomes, MaxOutcomeLength))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::TextRow(TEXT("Voting window (seconds)"), &Duration,
				FString::Printf(TEXT("%d to %d. The prediction locks when it runs out."), MinDuration, MaxDuration))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SampleUI::Indented(SampleUI::ActionButton(Context.ToSharedRef(), TEXT("Start prediction"), [this]() { StartPrediction(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Section(TEXT("Current prediction"), TEXT("Totals update live while viewers place bets.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(PredictionView, SVerticalBox) ]
	];

	RebuildPredictionView();
}

void SPredictionsPage::StartPrediction()
{
	TArray<FString> CleanOutcomes;
	for (const FString& Outcome : Outcomes)
	{
		const FString Trimmed = Outcome.TrimStartAndEnd();
		if (!Trimmed.IsEmpty())
			CleanOutcomes.Add(Trimmed);
	}

	FTwitchSampleContext& Ctx = *Context;
	if (!SampleUI::ValidateText(Ctx, Title, TEXT("Title"), MaxTitleLength))
		return;
	if (CleanOutcomes.Num() < MinOutcomes)
	{
		Ctx.Error(FString::Printf(TEXT("Enter at least %d outcomes."), MinOutcomes));
		return;
	}
	for (const FString& Outcome : CleanOutcomes)
	{
		if (Outcome.Len() > MaxOutcomeLength)
		{
			Ctx.Error(FString::Printf(TEXT("Outcome \"%s\" is longer than %d characters."), *Outcome, MaxOutcomeLength));
			return;
		}
	}
	int32 Seconds = 0;
	if (!SampleUI::TryParseInt(Ctx, Duration, TEXT("Voting window"), MinDuration, MaxDuration, Seconds))
		return;

	TwitchSDK::PredictionDefinition Definition;
	Definition.Title = ToOwned(Title.TrimStartAndEnd());
	Definition.Outcomes = ToOwned(CleanOutcomes);
	Definition.Duration = Seconds;

	const TWeakPtr<SPredictionsPage> WeakThis = SharedThis(this);
	Context->Call<TwitchSDK::PredictionInfo>(TEXT("Start prediction"),
		[Definition](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.CreatePrediction(Definition, Resolve, Reject); },
		[WeakThis, Seconds](const TwitchSDK::PredictionInfo& Info)
		{
			if (const TSharedPtr<SPredictionsPage> This = WeakThis.Pin())
			{
				This->ShowNewPrediction(Info);
				This->Context->Success(FString::Printf(TEXT("Prediction \"%s\" started. Voting closes in %d seconds."), *ToFString(Info.Title), Seconds));
			}
		});
}

void SPredictionsPage::ShowNewPrediction(const TwitchSDK::PredictionInfo& Info)
{
	// Keep the previous prediction until the new one exists, then stop following it.
	ClearPrediction();
	Prediction = Info;
	bUnsubscribed = false;
	bLoggedLock = false;
	RebuildPredictionView();
	WatchForUpdates(Generation);
}

void SPredictionsPage::WatchForUpdates(int32 MyGeneration)
{
	if (!Prediction.IsSet() || MyGeneration != Generation)
		return;

	if (Prediction->Status == TwitchSDK::PredictionStatus::Locked && !bLoggedLock)
	{
		bLoggedLock = true;
		Context->Info(FString::Printf(TEXT("Prediction \"%s\" is locked. Pick the winning outcome to pay out."), *ToFString(Prediction->Title)));
	}
	if (!IsOpen(Prediction->Status))
	{
		OnPredictionFinished();
		return;
	}

	const FString PredictionId = ToFString(Prediction->Id);
	const TWeakPtr<SPredictionsPage> WeakThis = SharedThis(this);
	Context->Call<TwitchSDK::PredictionInfo>(TEXT("Prediction updates"),
		[PredictionId](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.WaitForPredictionUpdate(FromFString(PredictionId), Resolve, Reject); },
		[WeakThis, MyGeneration](const TwitchSDK::PredictionInfo& Info)
		{
			const TSharedPtr<SPredictionsPage> This = WeakThis.Pin();
			if (!This || MyGeneration != This->Generation)
				return;
			This->Prediction = Info;
			This->RebuildPredictionView();
			This->WatchForUpdates(MyGeneration);
		},
		[WeakThis, MyGeneration](const FString& Message)
		{
			const TSharedPtr<SPredictionsPage> This = WeakThis.Pin();
			if (This && MyGeneration == This->Generation)
				This->Context->Warning(TEXT("Stopped receiving prediction updates: ") + Message);
		},
		/*bTrackAsRunning*/ false);
}

void SPredictionsPage::EndPrediction(TwitchSDK::PredictionStatus Status, const FString& WinningOutcomeId, const FString& Action, const FString& SuccessMessage)
{
	if (!Prediction.IsSet())
		return;

	TwitchSDK::EndPredictionRequest Request;
	Request.BroadcasterId = ToOwned(ToFString(Prediction->BroadcasterId));
	Request.PredictionId = ToOwned(ToFString(Prediction->Id));
	Request.Status = Status;
	Request.WinningOutcomeId = ToOwned(WinningOutcomeId);

	const TWeakPtr<SPredictionsPage> WeakThis = SharedThis(this);
	const int32 MyGeneration = Generation;
	Context->Call<TwitchSDK::PredictionInfo>(Action,
		[Request](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.EndPrediction(Request, Resolve, Reject); },
		[WeakThis, MyGeneration, SuccessMessage](const TwitchSDK::PredictionInfo& Info)
		{
			const TSharedPtr<SPredictionsPage> This = WeakThis.Pin();
			if (!This || MyGeneration != This->Generation)
				return;
			This->Prediction = Info;
			This->RebuildPredictionView();
			This->Context->Success(SuccessMessage);
			if (!IsOpen(Info.Status))
				This->OnPredictionFinished();
		});
}

void SPredictionsPage::OnPredictionFinished()
{
	if (!Prediction.IsSet() || bUnsubscribed)
		return;

	// The prediction gets no more updates. The core allows unsubscribing only once.
	bUnsubscribed = true;
	++Generation;
	if (const std::shared_ptr<FTwitchSampleContext::FApi> Api = Context->GetApi())
		Api->UnsubscribeFromPrediction(Prediction->Id);
	Context->Info(FString::Printf(TEXT("Prediction \"%s\" finished: %s."), *ToFString(Prediction->Title), *StatusName(Prediction->Status)));
}

void SPredictionsPage::ClearPrediction()
{
	++Generation;
	if (Prediction.IsSet() && !bUnsubscribed)
	{
		bUnsubscribed = true;
		if (const std::shared_ptr<FTwitchSampleContext::FApi> Api = Context->GetApi())
			Api->UnsubscribeFromPrediction(Prediction->Id);
	}
	Prediction.Reset();
	RebuildPredictionView();
}

void SPredictionsPage::RebuildPredictionView()
{
	if (!PredictionView.IsValid())
		return;
	PredictionView->ClearChildren();

	if (!Prediction.IsSet())
	{
		PredictionView->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("No prediction yet. Start one above."))) ];
		return;
	}

	const TwitchSDK::PredictionInfo& Info = *Prediction;
	const bool bOpen = IsOpen(Info.Status);

	int64 TotalPoints = 0;
	for (const TwitchSDK::PredictionOutcome& Outcome : Info.Outcomes)
		TotalPoints += Outcome.ChannelPoints;

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Title"), ToFString(Info.Title)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Status"), StatusName(Info.Status)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Created"), ToFString(Info.CreatedAt)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Locked"), ToFString(Info.LockedAt)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Ended"), ToFString(Info.EndedAt)) ];

	const FString WinnerId = ToFString(Info.WinningOutcomeId);
	for (const TwitchSDK::PredictionOutcome& Outcome : Info.Outcomes)
	{
		const FString OutcomeId = ToFString(Outcome.Id);
		const FString OutcomeTitle = ToFString(Outcome.Title);
		const bool bWinner = !WinnerId.IsEmpty() && OutcomeId == WinnerId;
		const FString Action = TEXT("Resolve: ") + OutcomeTitle;

		Box->AddSlot().AutoHeight().Padding(0, 3)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(SampleUI::LabelWidth)
				[
					SampleUI::Text(FText::FromString((bWinner ? TEXT("Winner: ") : TEXT("")) + OutcomeTitle))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SampleUI::Bar(TotalPoints > 0 ? static_cast<float>(Outcome.ChannelPoints) / static_cast<float>(TotalPoints) : 0.f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8, 0)
			[
				SampleUI::Text(FText::FromString(FString::Printf(TEXT("%lld viewers, %lld points (%s)"),
					static_cast<long long>(Outcome.Users), static_cast<long long>(Outcome.ChannelPoints), *ToFString(Outcome.Color))))
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SampleUI::ActionButton(Context.ToSharedRef(), Action, [this, OutcomeId, OutcomeTitle, Action]()
					{
						EndPrediction(TwitchSDK::PredictionStatus::Resolved, OutcomeId, Action,
							FString::Printf(TEXT("Prediction resolved. \"%s\" won and points were paid out."), *OutcomeTitle));
					},
					bOpen)
			]
		];
	}

	Box->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SampleUI::ActionButton(Context.ToSharedRef(), TEXT("Lock voting now"), [this]()
				{
					EndPrediction(TwitchSDK::PredictionStatus::Locked, FString(), TEXT("Lock voting now"),
						TEXT("Prediction locked. Viewers can no longer place bets."));
				},
				Info.Status == TwitchSDK::PredictionStatus::Active)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SampleUI::ActionButton(Context.ToSharedRef(), TEXT("Cancel prediction (refund points)"), [this]()
				{
					EndPrediction(TwitchSDK::PredictionStatus::Canceled, FString(), TEXT("Cancel prediction (refund points)"),
						TEXT("Prediction canceled. Channel points were refunded."));
				},
				bOpen)
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SampleUI::Button(TEXT("Clear"), [this]() { ClearPrediction(); })
		]
	];

	PredictionView->AddSlot().AutoHeight()[ SampleUI::Panel(Box) ];
}
