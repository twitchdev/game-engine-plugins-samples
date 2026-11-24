#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FTwitchSampleContext;
class SSamplePage;
class SVerticalBox;

/**
 * The whole sample UI: a header, one tab per page, the current page, and the log.
 */
class STwitchSampleRoot : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STwitchSampleRoot) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext);
	virtual ~STwitchSampleRoot() override;

	/** Shows the page at Index (0 = Authorization). */
	void SetPage(int32 Index);
	int32 GetPageCount() const { return Pages.Num(); }
	TSharedPtr<SSamplePage> GetPage(int32 Index) const { return Pages.IsValidIndex(Index) ? Pages[Index] : nullptr; }

	/** Stops everything the pages created from the API. */
	void ReleaseApiResources();

	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	TSharedRef<SWidget> MakeHeader();
	TSharedRef<SWidget> MakeTabs();
	TSharedRef<SWidget> MakeLog();
	void RebuildLog();

	TSharedPtr<FTwitchSampleContext> Context;
	TArray<TSharedPtr<SSamplePage>> Pages;
	int32 CurrentPage = 0;
	TSharedPtr<SVerticalBox> LogList;
	FDelegateHandle LogChangedHandle;
};
