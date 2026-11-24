#include "UI/SStringListEditor.h"

#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#include "UI/SampleWidgets.h"

void SStringListEditor::Construct(const FArguments& InArgs)
{
	Items = InArgs._Items;
	ItemLabel = InArgs._ItemLabel;
	MinItems = InArgs._MinItems;
	MaxItems = InArgs._MaxItems;
	Hint = InArgs._Hint;
	check(Items);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SAssignNew(RowsBox, SVerticalBox) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SampleUI::IndentedFill(
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
				[
					SampleUI::Button(TEXT("+ Add ") + ItemLabel.ToLower(), [this]()
						{
							Items->Add(FString());
							Rebuild();
						},
						TAttribute<bool>::CreateLambda([this]() { return Items->Num() < MaxItems; }))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SampleUI::Muted(FText::FromString(Hint))
				])
		]
	];

	Rebuild();
}

void SStringListEditor::Rebuild()
{
	RowsBox->ClearChildren();
	for (int32 Index = 0; Index < Items->Num(); ++Index)
	{
		RowsBox->AddSlot().AutoHeight().Padding(0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(SampleUI::LabelWidth)
				[
					SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("%s %d"), *ItemLabel, Index + 1))).Font(SampleUI::Font(10))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SEditableTextBox)
				.Font(SampleUI::Font(10))
				.Text(FText::FromString((*Items)[Index]))
				.OnTextChanged_Lambda([this, Index](const FText& NewText)
				{
					if (Items->IsValidIndex(Index))
						(*Items)[Index] = NewText.ToString();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0, 0, 0)
			[
				SampleUI::Button(TEXT("Remove"), [this, Index]()
					{
						if (Items->IsValidIndex(Index) && Items->Num() > MinItems)
						{
							Items->RemoveAt(Index);
							Rebuild();
						}
					},
					TAttribute<bool>::CreateLambda([this]() { return Items->Num() > MinItems; }))
			]
		];
	}
}
