#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

#include "UI/SSamplePage.h"

/**
 * Create a poll on your channel from your own question and choices, watch votes come in, and end it.
 */
class SPollsPage : public SSamplePage
{
public:
	SLATE_BEGIN_ARGS(SPollsPage) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<FTwitchSampleContext> InContext);

	virtual FString GetTitle() const override { return TEXT("Polls"); }
	virtual void ReleaseApiResources() override { ClearPoll(); }

private:
	void StartPoll();
	void ShowNewPoll(const TwitchSDK::PollInfo& Info);
	void WatchForUpdates(int32 MyGeneration);
	void EndPoll(bool bShowResults);
	void OnPollFinished();
	void ClearPoll();
	void RebuildPollView();

	FString Question = TEXT("Which path should we take?");
	TArray<FString> Choices = { TEXT("Left"), TEXT("Right") };
	FString Duration = TEXT("60");
	bool bChannelPointsVoting = false;
	FString ChannelPointsPerVote = TEXT("100");

	TOptional<TwitchSDK::PollInfo> Poll;
	bool bUnsubscribed = false;
	int32 Generation = 0;

	TSharedPtr<SVerticalBox> PollView;
};
