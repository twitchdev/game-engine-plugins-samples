#include "UI/Pages/SPollsPage.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"

#include "UI/SampleWidgets.h"
#include "UI/SStringListEditor.h"

using TwitchSDK::ToFString;
using TwitchSDK::FromFString;

namespace
{
	// Limits enforced by the Twitch Create Poll endpoint.
	constexpr int32 MaxQuestionLength = 60;
	constexpr int32 MaxChoiceLength = 25;
	constexpr int32 MinChoices = 2;
	constexpr int32 MaxChoices = 5;
	constexpr int32 MinDuration = 15;
	constexpr int32 MaxDuration = 1800;

	FString StatusName(TwitchSDK::PollStatus Status)
	{
		switch (Status)
		{
		case TwitchSDK::PollStatus::Active: return TEXT("Active");
		case TwitchSDK::PollStatus::Completed: return TEXT("Completed");
		case TwitchSDK::PollStatus::Terminated: return TEXT("Ended early (results shown)");
		case TwitchSDK::PollStatus::Archived: return TEXT("Canceled (results hidden)");
		case TwitchSDK::PollStatus::Moderated: return TEXT("Moderated");
		default: return TEXT("Invalid");
		}
	}

	FString Winner(const TwitchSDK::PollInfo& Info)
	{
		const TwitchSDK::PollChoiceInfo* Top = nullptr;
		for (const TwitchSDK::PollChoiceInfo& Choice : Info.Choices)
		{
			if (!Top || Choice.Votes > Top->Votes)
				Top = &Choice;
		}
		if (!Top || Top->Votes == 0)
			return TEXT("No votes were cast.");
		return FString::Printf(TEXT("Most votes: \"%s\" with %lld."), *ToFString(Top->Title), static_cast<long long>(Top->Votes));
	}
}

void SPollsPage::Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext)
{
	Context = InContext;

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeConnectionHint() ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("Create a poll"),
				TEXT("Runs a poll on your channel. You need to be a Twitch Affiliate or Partner, and Twitch allows one active poll at a time."))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Question"), &Question, FString::Printf(TEXT("Up to %d characters."), MaxQuestionLength)) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SStringListEditor)
			.Items(&Choices)
			.ItemLabel(TEXT("Choice"))
			.MinItems(MinChoices)
			.MaxItems(MaxChoices)
			.Hint(FString::Printf(TEXT("%d to %d choices, up to %d characters each."), MinChoices, MaxChoices, MaxChoiceLength))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Duration (seconds)"), &Duration, FString::Printf(TEXT("%d to %d."), MinDuration, MaxDuration)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Indented(SampleUI::CheckBox(TEXT("Allow extra votes with channel points"), &bChannelPointsVoting)) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::TextRow(TEXT("Channel points per extra vote"), &ChannelPointsPerVote, FString(),
				TAttribute<bool>::CreateLambda([this]() { return bChannelPointsVoting; }))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SampleUI::Indented(SampleUI::ActionButton(Context.ToSharedRef(), TEXT("Start poll"), [this]() { StartPoll(); }))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Section(TEXT("Current poll"), TEXT("Vote counts update live while the poll runs.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(PollView, SVerticalBox) ]
	];

	RebuildPollView();
}

void SPollsPage::StartPoll()
{
	TArray<FString> CleanChoices;
	for (const FString& Choice : Choices)
	{
		const FString Trimmed = Choice.TrimStartAndEnd();
		if (!Trimmed.IsEmpty())
			CleanChoices.Add(Trimmed);
	}

	FTwitchSampleContext& Ctx = *Context;
	if (!SampleUI::ValidateText(Ctx, Question, TEXT("Question"), MaxQuestionLength))
		return;
	if (CleanChoices.Num() < MinChoices)
	{
		Ctx.Error(FString::Printf(TEXT("Enter at least %d choices."), MinChoices));
		return;
	}
	for (const FString& Choice : CleanChoices)
	{
		if (Choice.Len() > MaxChoiceLength)
		{
			Ctx.Error(FString::Printf(TEXT("Choice \"%s\" is longer than %d characters."), *Choice, MaxChoiceLength));
			return;
		}
	}
	int32 Seconds = 0;
	if (!SampleUI::TryParseInt(Ctx, Duration, TEXT("Duration"), MinDuration, MaxDuration, Seconds))
		return;
	int32 Points = 0;
	if (bChannelPointsVoting && !SampleUI::TryParseInt(Ctx, ChannelPointsPerVote, TEXT("Channel points per vote"), 1, 1000000, Points))
		return;

	TwitchSDK::PollDefinition Definition;
	Definition.Title = ToOwned(Question.TrimStartAndEnd());
	Definition.Choices = ToOwned(CleanChoices);
	Definition.Duration = Seconds;
	Definition.ChannelPointsVotingEnabled = bChannelPointsVoting;
	Definition.ChannelPointsPerVote = Points;

	const TWeakPtr<SPollsPage> WeakThis = SharedThis(this);
	Context->Call<TwitchSDK::PollInfo>(TEXT("Start poll"),
		[Definition](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.CreatePoll(Definition, Resolve, Reject); },
		[WeakThis, Seconds](const TwitchSDK::PollInfo& Info)
		{
			if (const TSharedPtr<SPollsPage> This = WeakThis.Pin())
			{
				This->ShowNewPoll(Info);
				This->Context->Success(FString::Printf(TEXT("Poll \"%s\" started for %d seconds."), *ToFString(Info.Title), Seconds));
			}
		});
}

void SPollsPage::ShowNewPoll(const TwitchSDK::PollInfo& Info)
{
	// Keep the previous poll until the new one exists, then stop following it.
	ClearPoll();
	Poll = Info;
	bUnsubscribed = false;
	RebuildPollView();
	WatchForUpdates(Generation);
}

void SPollsPage::WatchForUpdates(int32 MyGeneration)
{
	if (!Poll.IsSet() || MyGeneration != Generation)
		return;
	if (Poll->Status != TwitchSDK::PollStatus::Active)
	{
		OnPollFinished();
		return;
	}

	const FString PollId = ToFString(Poll->Id);
	const TWeakPtr<SPollsPage> WeakThis = SharedThis(this);
	Context->Call<TwitchSDK::PollInfo>(TEXT("Poll updates"),
		[PollId](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.WaitForPollUpdate(FromFString(PollId), Resolve, Reject); },
		[WeakThis, MyGeneration](const TwitchSDK::PollInfo& Info)
		{
			const TSharedPtr<SPollsPage> This = WeakThis.Pin();
			if (!This || MyGeneration != This->Generation)
				return;
			This->Poll = Info;
			This->RebuildPollView();
			This->WatchForUpdates(MyGeneration);
		},
		[WeakThis, MyGeneration](const FString& Message)
		{
			const TSharedPtr<SPollsPage> This = WeakThis.Pin();
			if (This && MyGeneration == This->Generation)
				This->Context->Warning(TEXT("Stopped receiving poll updates: ") + Message);
		},
		/*bTrackAsRunning*/ false);
}

void SPollsPage::EndPoll(bool bShowResults)
{
	if (!Poll.IsSet())
		return;

	TwitchSDK::EndPollRequest Request;
	Request.BroadcasterId = ToOwned(ToFString(Poll->BroadcasterId));
	Request.PollId = ToOwned(ToFString(Poll->Id));
	Request.ShowResults = bShowResults;

	const FString Action = bShowResults ? TEXT("End poll and show results") : TEXT("Cancel poll (hide results)");
	const TWeakPtr<SPollsPage> WeakThis = SharedThis(this);
	const int32 MyGeneration = Generation;
	Context->Call<TwitchSDK::PollInfo>(Action,
		[Request](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.EndPoll(Request, Resolve, Reject); },
		[WeakThis, MyGeneration, bShowResults](const TwitchSDK::PollInfo& Info)
		{
			const TSharedPtr<SPollsPage> This = WeakThis.Pin();
			if (!This || MyGeneration != This->Generation)
				return;
			This->Poll = Info;
			This->RebuildPollView();
			This->Context->Success(bShowResults ? TEXT("Poll ended. Viewers can see the results.") : TEXT("Poll canceled. No results are shown to viewers."));
			This->OnPollFinished();
		});
}

void SPollsPage::OnPollFinished()
{
	if (!Poll.IsSet() || bUnsubscribed)
		return;

	// The poll gets no more updates, so stop following it. The core allows unsubscribing only once.
	bUnsubscribed = true;
	++Generation;
	if (const std::shared_ptr<FTwitchSampleContext::FApi> Api = Context->GetApi())
		Api->UnsubscribeFromPoll(Poll->Id);
	Context->Info(FString::Printf(TEXT("Poll \"%s\" finished: %s. %s"), *ToFString(Poll->Title), *StatusName(Poll->Status), *Winner(*Poll)));
}

void SPollsPage::ClearPoll()
{
	++Generation;
	if (Poll.IsSet() && !bUnsubscribed)
	{
		bUnsubscribed = true;
		if (const std::shared_ptr<FTwitchSampleContext::FApi> Api = Context->GetApi())
			Api->UnsubscribeFromPoll(Poll->Id);
	}
	Poll.Reset();
	RebuildPollView();
}

void SPollsPage::RebuildPollView()
{
	if (!PollView.IsValid())
		return;
	PollView->ClearChildren();

	if (!Poll.IsSet())
	{
		PollView->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("No poll yet. Start one above."))) ];
		return;
	}

	const TwitchSDK::PollInfo& Info = *Poll;
	const bool bActive = Info.Status == TwitchSDK::PollStatus::Active;

	int64 Total = 0;
	for (const TwitchSDK::PollChoiceInfo& Choice : Info.Choices)
		Total += Choice.Votes;

	TSharedRef<SVerticalBox> Box = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Question"), ToFString(Info.Title)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Status"), StatusName(Info.Status)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Started"), ToFString(Info.StartedAt)) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Ended"), bActive ? FString() : ToFString(Info.EndedAt)) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::KeyValue(TEXT("Channel point voting"), Info.ChannelPointsVotingEnabled
				? FString::Printf(TEXT("On, %d points per vote"), Info.ChannelPointsPerVote) : FString(TEXT("Off")))
		];

	for (const TwitchSDK::PollChoiceInfo& Choice : Info.Choices)
	{
		Box->AddSlot().AutoHeight().Padding(0, 3)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(SampleUI::LabelWidth)[ SampleUI::Text(FText::FromString(ToFString(Choice.Title))) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SampleUI::Bar(Total > 0 ? static_cast<float>(Choice.Votes) / static_cast<float>(Total) : 0.f)
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8, 0, 0, 0)
			[
				SampleUI::Text(FText::FromString(FString::Printf(TEXT("%lld votes (%lld with channel points)"),
					static_cast<long long>(Choice.Votes), static_cast<long long>(Choice.ChannelPointsVotes))))
			]
		];
	}

	Box->AddSlot().AutoHeight()[ SampleUI::KeyValue(TEXT("Total votes"), FString::Printf(TEXT("%lld"), static_cast<long long>(Total))) ];
	Box->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SampleUI::ActionButton(Context.ToSharedRef(), TEXT("End poll and show results"), [this]() { EndPoll(true); }, bActive)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
		[
			SampleUI::ActionButton(Context.ToSharedRef(), TEXT("Cancel poll (hide results)"), [this]() { EndPoll(false); }, bActive)
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			SampleUI::Button(TEXT("Clear"), [this]() { ClearPoll(); })
		]
	];

	PollView->AddSlot().AutoHeight()[ SampleUI::Panel(Box) ];
}
