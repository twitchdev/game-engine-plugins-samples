#include "UI/Pages/SDropsPage.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/SampleWidgets.h"

using TwitchSDK::ToFString;

namespace
{
	const TCHAR* StatusOptions[] = { TEXT("Claimed (not granted yet)"), TEXT("Fulfilled"), TEXT("All") };

	// Unknown means "no filter" in the core library.
	const TwitchSDK::DropFulfillmentStatus StatusValues[] = {
		TwitchSDK::DropFulfillmentStatus::Claimed,
		TwitchSDK::DropFulfillmentStatus::Fulfilled,
		TwitchSDK::DropFulfillmentStatus::Unknown,
	};

	FString StatusName(TwitchSDK::DropFulfillmentStatus Status)
	{
		switch (Status)
		{
		case TwitchSDK::DropFulfillmentStatus::Claimed: return TEXT("Claimed");
		case TwitchSDK::DropFulfillmentStatus::Fulfilled: return TEXT("Fulfilled");
		default: return TEXT("Unknown");
		}
	}
}

void SDropsPage::Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext)
{
	Context = InContext;
	const TSharedRef<FTwitchSampleContext> Ctx = InContext;

	TSharedRef<SHorizontalBox> StatusChoices = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < static_cast<int32>(UE_ARRAY_COUNT(StatusOptions)); ++Index)
	{
		StatusChoices->AddSlot().AutoWidth().Padding(0, 0, 12, 0)
		[
			SNew(SCheckBox)
			.Style(FCoreStyle::Get(), "RadioButton")
			.IsChecked_Lambda([this, Index]() { return StatusIndex == Index ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([this, Index](ECheckBoxState) { StatusIndex = Index; })
			[
				SNew(STextBlock).Text(FText::FromString(StatusOptions[Index])).Font(SampleUI::Font(10))
			]
		];
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ MakeConnectionHint() ]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("Your reward IDs"),
				TEXT("The benefit IDs of the rewards in your Drops campaign (Twitch developer console > Drops > Rewards). The sample treats "
					 "these as the items your game knows how to grant. Separate several IDs with commas."))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Reward (benefit) IDs"), &RewardIds) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Row(TEXT("Rewards entered"), SampleUI::Text(TAttribute<FText>::CreateLambda([this]()
			{
				return FText::AsNumber(SampleUI::SplitList(RewardIds).Num());
			})))
		]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("Check for Drops"),
				TEXT("Lists Drops the logged-in user earned for games owned by your Client ID's organization. No extra scope is needed. "
					 "There is no live notification for Drops, so check when it makes sense in your game."))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Game ID (optional)"), &GameId, TEXT("Leave blank to include all of your games.")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Row(TEXT("Show"), StatusChoices) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SampleUI::Indented(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
				[
					SampleUI::ActionButton(Ctx, TEXT("Check for Drops"), [this]() { CheckForDrops(); })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SampleUI::ActionButton(Ctx, TEXT("Grant and fulfill matching rewards"), [this]() { GrantMatching(); },
						TAttribute<bool>::CreateLambda([this]() { return Entitlements.IsSet() && SampleUI::SplitList(RewardIds).Num() > 0; }))
				])
		]

		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::Section(TEXT("Results")) ]
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(ResultsBox, SVerticalBox) ]

		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::Section(TEXT("Fulfill by entitlement ID"),
				TEXT("Marks specific entitlements fulfilled without checking first. Separate several IDs with commas."))
		]
		+ SVerticalBox::Slot().AutoHeight()[ SampleUI::TextRow(TEXT("Entitlement IDs"), &ManualEntitlementIds) ]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 6)
		[
			SampleUI::Indented(SampleUI::ActionButton(Ctx, TEXT("Fulfill these entitlements"), [this]()
				{
					const TArray<FString> Ids = SampleUI::SplitList(ManualEntitlementIds);
					Fulfill(TEXT("Fulfill these entitlements"), Ids,
						FString::Printf(TEXT("Sent %d entitlement(s) to Twitch to mark fulfilled. Check for Drops to confirm."), Ids.Num()),
						/*bRefreshAfter*/ false);
				},
				TAttribute<bool>::CreateLambda([this]() { return SampleUI::SplitList(ManualEntitlementIds).Num() > 0; })))
		]
	];

	RebuildResults();
}

void SDropsPage::ReleaseApiResources()
{
	Entitlements.Reset();
	LastQueryDescription.Reset();
	RebuildResults();
}

void SDropsPage::CheckForDrops()
{
	TwitchSDK::DropEntitlementQuery Query;
	const FString Game = GameId.TrimStartAndEnd();
	Query.GameId = ToOwned(Game);
	Query.FulfillmentStatus = StatusValues[StatusIndex];

	const FString Description = FString::Printf(TEXT("%s, %s, checked at %s"),
		StatusOptions[StatusIndex], Game.IsEmpty() ? TEXT("all games") : *(TEXT("game ") + Game), *FDateTime::Now().ToString(TEXT("%H:%M:%S")));

	const TWeakPtr<SDropsPage> WeakThis = SharedThis(this);
	Context->Call<TwitchSDK::DropEntitlementList>(TEXT("Check for Drops"),
		[Query](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.GetDropEntitlements(Query, Resolve, Reject); },
		[WeakThis, Description](const TwitchSDK::DropEntitlementList& Result)
		{
			const TSharedPtr<SDropsPage> This = WeakThis.Pin();
			if (!This)
				return;
			TArray<TwitchSDK::DropEntitlement> Found;
			for (const TwitchSDK::DropEntitlement& Entitlement : Result.Entitlements)
				Found.Add(Entitlement);
			This->Entitlements = MoveTemp(Found);
			This->LastQueryDescription = Description;
			This->RebuildResults();
			This->Context->Success(FString::Printf(TEXT("Found %d Drop entitlement(s)."), This->Entitlements->Num()));
		});
}

void SDropsPage::GrantMatching()
{
	if (!Entitlements.IsSet())
		return;

	const TSet<FString> Known(SampleUI::SplitList(RewardIds));
	TArray<FString> Matching;
	for (const TwitchSDK::DropEntitlement& Entitlement : *Entitlements)
	{
		if (Entitlement.FulfillmentStatus == TwitchSDK::DropFulfillmentStatus::Claimed && Known.Contains(ToFString(Entitlement.BenefitId)))
		{
			// A real game adds the item to the player's inventory here, and only fulfills the
			// entitlements it actually granted so the rest are retried next time.
			Context->Info(FString::Printf(TEXT("Granting the in-game item for reward %s (entitlement %s)."),
				*ToFString(Entitlement.BenefitId), *ToFString(Entitlement.Id)));
			Matching.Add(ToFString(Entitlement.Id));
		}
	}

	if (Matching.Num() == 0)
	{
		Context->Info(TEXT("None of the claimed Drops match your reward IDs, so there is nothing to grant."));
		return;
	}

	Fulfill(TEXT("Grant and fulfill matching rewards"), Matching,
		FString::Printf(TEXT("Marked %d entitlement(s) fulfilled."), Matching.Num()), /*bRefreshAfter*/ true);
}

void SDropsPage::Fulfill(const FString& Action, const TArray<FString>& EntitlementIds, const FString& SuccessMessage, bool bRefreshAfter)
{
	if (EntitlementIds.Num() == 0)
		return;

	TwitchSDK::FulfillDropEntitlementsRequest Request;
	Request.EntitlementIds = ToOwned(EntitlementIds);

	const TWeakPtr<SDropsPage> WeakThis = SharedThis(this);
	Context->CallVoid(Action,
		[Request](FTwitchSampleContext::FApi& Api, auto Resolve, auto Reject) { Api.FulfillDropEntitlements(Request, Resolve, Reject); },
		[WeakThis, SuccessMessage, bRefreshAfter]()
		{
			if (const TSharedPtr<SDropsPage> This = WeakThis.Pin())
			{
				This->Context->Success(SuccessMessage);
				if (bRefreshAfter)
					This->CheckForDrops();
			}
		});
}

void SDropsPage::RebuildResults()
{
	if (!ResultsBox.IsValid())
		return;
	ResultsBox->ClearChildren();

	if (!Entitlements.IsSet())
	{
		ResultsBox->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(TEXT("Press Check for Drops to load the logged-in user's entitlements."))) ];
		return;
	}

	ResultsBox->AddSlot().AutoHeight()[ SampleUI::Muted(FText::FromString(LastQueryDescription)) ];
	if (Entitlements->Num() == 0)
	{
		ResultsBox->AddSlot().AutoHeight()
		[
			SampleUI::Text(FText::FromString(TEXT("No entitlements found. If you expected some, check the filters and that the Drops campaign "
												  "belongs to the organization that owns your Client ID.")))
		];
		return;
	}

	const TSet<FString> Known(SampleUI::SplitList(RewardIds));
	for (const TwitchSDK::DropEntitlement& Entitlement : *Entitlements)
	{
		const FString Id = ToFString(Entitlement.Id);
		const FString BenefitId = ToFString(Entitlement.BenefitId);
		const bool bClaimed = Entitlement.FulfillmentStatus == TwitchSDK::DropFulfillmentStatus::Claimed;
		const FString Action = TEXT("Fulfill ") + Id;

		ResultsBox->AddSlot().AutoHeight().Padding(0, 3)
		[
			SampleUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Entitlement ID"), Id) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Reward (benefit) ID"), BenefitId + (Known.Contains(BenefitId) ? TEXT("   (matches your reward IDs)") : TEXT(""))) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Status"), StatusName(Entitlement.FulfillmentStatus)) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Game ID"), ToFString(Entitlement.GameId)) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Granted"), ToFString(Entitlement.Timestamp)) ]
				+ SVerticalBox::Slot().AutoHeight()[ SampleUI::KeyValue(TEXT("Last updated"), ToFString(Entitlement.LastUpdated)) ]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
				[
					SampleUI::Indented(SampleUI::ActionButton(Context.ToSharedRef(), Action, [this, Action, Id]()
						{
							Fulfill(Action, { Id }, FString::Printf(TEXT("Marked entitlement %s fulfilled."), *Id), /*bRefreshAfter*/ true);
						},
						bClaimed))
				])
		];
	}
}
