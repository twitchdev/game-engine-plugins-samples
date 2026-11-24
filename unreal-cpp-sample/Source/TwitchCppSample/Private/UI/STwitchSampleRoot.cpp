#include "UI/STwitchSampleRoot.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/Pages/SAuthorizationPage.h"
#include "UI/Pages/SDropsPage.h"
#include "UI/Pages/SGeneralPage.h"
#include "UI/Pages/SPollsPage.h"
#include "UI/Pages/SPredictionsPage.h"
#include "UI/SampleWidgets.h"
#include "UI/TwitchSampleContext.h"

namespace
{
	constexpr float LogHeight = 200.f;
	constexpr int32 MaxVisibleLogEntries = 150;
}

void STwitchSampleRoot::Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext)
{
	Context = InContext;

	Pages.Add(SNew(SAuthorizationPage, InContext));
	Pages.Add(SNew(SPollsPage, InContext));
	Pages.Add(SNew(SPredictionsPage, InContext));
	Pages.Add(SNew(SDropsPage, InContext));
	Pages.Add(SNew(SGeneralPage, InContext));

	TSharedRef<SWidgetSwitcher> Switcher = SNew(SWidgetSwitcher)
		.WidgetIndex_Lambda([this]() { return CurrentPage; });
	for (const TSharedPtr<SSamplePage>& Page : Pages)
	{
		Switcher->AddSlot()
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot().Padding(FMargin(4, 0, 16, 16))
			[
				Page.ToSharedRef()
			]
		];
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(SampleUI::BackgroundColor())
		.Padding(12)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()[ MakeHeader() ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8)[ MakeTabs() ]
			+ SVerticalBox::Slot().FillHeight(1.f)[ Switcher ]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)[ MakeLog() ]
		]
	];

	LogChangedHandle = Context->OnLogChanged.AddSP(this, &STwitchSampleRoot::RebuildLog);
	RebuildLog();
}

STwitchSampleRoot::~STwitchSampleRoot()
{
	if (Context.IsValid())
		Context->OnLogChanged.Remove(LogChangedHandle);
}

void STwitchSampleRoot::SetPage(int32 Index)
{
	if (Pages.IsValidIndex(Index))
		CurrentPage = Index;
}

void STwitchSampleRoot::ReleaseApiResources()
{
	for (const TSharedPtr<SSamplePage>& Page : Pages)
		Page->ReleaseApiResources();
}

TSharedRef<SWidget> STwitchSampleRoot::MakeHeader()
{
	const TWeakPtr<FTwitchSampleContext> WeakContext = Context;
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(FText::FromString(TEXT("Twitch SDK Unreal C++ Sample")))
			.Font(SampleUI::Font(20, true))
			.ColorAndOpacity(SampleUI::AccentColor())
		]
		+ SHorizontalBox::Slot().AutoWidth().HAlign(HAlign_Right)
		[
			// Short lines in an auto-width slot: no wrapping, or they collapse into a narrow column.
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Font(SampleUI::Font(10))
				.ColorAndOpacity(SampleUI::MutedColor())
				.Text(FText::FromString(TEXT("TwitchSDK plugin ") + Context->GetPluginVersion()))
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Font(SampleUI::Font(10))
				.ColorAndOpacity(SampleUI::MutedColor())
				.Text_Lambda([WeakContext]()
				{
					const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin();
					return Ctx ? FText::Format(FText::FromString(TEXT("Status: {0}")), Ctx->GetStatusText()) : FText::GetEmpty();
				})
			]
		];
}

TSharedRef<SWidget> STwitchSampleRoot::MakeTabs()
{
	TSharedRef<SHorizontalBox> Tabs = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < Pages.Num(); ++Index)
	{
		Tabs->AddSlot().FillWidth(1.f).Padding(2, 0)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ButtonColorAndOpacity_Lambda([this, Index]()
			{
				return Index == CurrentPage ? SampleUI::AccentColor() : FLinearColor(0.3f, 0.3f, 0.34f);
			})
			.OnClicked_Lambda([this, Index]()
			{
				SetPage(Index);
				return FReply::Handled();
			})
			[
				SNew(STextBlock).Text(FText::FromString(Pages[Index]->GetTitle())).Font(SampleUI::Font(11, true))
			]
		];
	}
	return Tabs;
}

TSharedRef<SWidget> STwitchSampleRoot::MakeLog()
{
	return SNew(SBox).HeightOverride(LogHeight)
		[
			SampleUI::Panel(
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("Log (newest first)"))).Font(SampleUI::Font(12, true))
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SampleUI::Button(TEXT("Clear"), [WeakContext = TWeakPtr<FTwitchSampleContext>(Context)]()
						{
							if (const TSharedPtr<FTwitchSampleContext> Ctx = WeakContext.Pin())
								Ctx->ClearLog();
						})
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(0, 4, 0, 0)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					[
						SAssignNew(LogList, SVerticalBox)
					]
				])
		];
}

void STwitchSampleRoot::RebuildLog()
{
	if (!LogList.IsValid() || !Context.IsValid())
		return;

	LogList->ClearChildren();
	const TArray<FSampleLogEntry>& Entries = Context->GetLogEntries();
	for (int32 Index = 0; Index < Entries.Num() && Index < MaxVisibleLogEntries; ++Index)
	{
		const FSampleLogEntry& Entry = Entries[Index];
		LogList->AddSlot().AutoHeight().Padding(0, 1)
		[
			SampleUI::Text(
				FText::FromString(FString::Printf(TEXT("%s   %s"), *Entry.Time.ToString(TEXT("%H:%M:%S")), *Entry.Message)),
				SampleUI::LevelColor(Entry.Level))
		];
	}
}
