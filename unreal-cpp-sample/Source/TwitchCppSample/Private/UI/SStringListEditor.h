#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;

/**
 * Edits a list of strings: one numbered text box per item, plus Add and Remove buttons that keep
 * the count between MinItems and MaxItems. The list itself is owned by the caller (a page member).
 */
class SStringListEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStringListEditor)
		: _Items(nullptr), _MinItems(1), _MaxItems(10)
	{}
		SLATE_ARGUMENT(TArray<FString>*, Items)
		SLATE_ARGUMENT(FString, ItemLabel)
		SLATE_ARGUMENT(int32, MinItems)
		SLATE_ARGUMENT(int32, MaxItems)
		SLATE_ARGUMENT(FString, Hint)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void Rebuild();

	TArray<FString>* Items = nullptr;
	FString ItemLabel;
	int32 MinItems = 1;
	int32 MaxItems = 10;
	FString Hint;
	TSharedPtr<SVerticalBox> RowsBox;
};
