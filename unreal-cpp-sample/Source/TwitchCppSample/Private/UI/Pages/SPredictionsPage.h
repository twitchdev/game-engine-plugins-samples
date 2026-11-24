#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

#include "UI/SSamplePage.h"

/**
 * Create a prediction from your own title and outcomes, then lock, resolve, or cancel it.
 */
class SPredictionsPage : public SSamplePage
{
public:
	SLATE_BEGIN_ARGS(SPredictionsPage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext);

	virtual FString GetTitle() const override { return TEXT("Predictions"); }
	virtual void ReleaseApiResources() override { ClearPrediction(); }

private:
	void StartPrediction();
	void ShowNewPrediction(const TwitchSDK::PredictionInfo& Info);
	void WatchForUpdates(int32 MyGeneration);
	void EndPrediction(TwitchSDK::PredictionStatus Status, const FString& WinningOutcomeId, const FString& Action, const FString& SuccessMessage);
	void OnPredictionFinished();
	void ClearPrediction();
	void RebuildPredictionView();

	FString Title = TEXT("Will we beat the boss?");
	TArray<FString> Outcomes = { TEXT("Yes"), TEXT("No") };
	FString Duration = TEXT("120");

	TOptional<TwitchSDK::PredictionInfo> Prediction;
	bool bUnsubscribed = false;
	bool bLoggedLock = false;
	int32 Generation = 0;

	TSharedPtr<SVerticalBox> PredictionView;
};
