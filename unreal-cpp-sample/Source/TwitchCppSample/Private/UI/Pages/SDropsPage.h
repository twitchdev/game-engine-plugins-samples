#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

#include "UI/SSamplePage.h"

/**
 * Check the logged-in user's Drops, grant the rewards this "game" knows about, and mark them fulfilled.
 */
class SDropsPage : public SSamplePage
{
public:
	SLATE_BEGIN_ARGS(SDropsPage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext);

	virtual FString GetTitle() const override { return TEXT("Drops"); }
	virtual void ReleaseApiResources() override;

private:
	void CheckForDrops();
	void GrantMatching();
	void Fulfill(const FString& Action, const TArray<FString>& EntitlementIds, const FString& SuccessMessage, bool bRefreshAfter);
	void RebuildResults();

	FString RewardIds;
	FString GameId;
	int32 StatusIndex = 0;
	FString ManualEntitlementIds;

	TOptional<TArray<TwitchSDK::DropEntitlement>> Entitlements;
	FString LastQueryDescription;

	TSharedPtr<SVerticalBox> ResultsBox;
};
