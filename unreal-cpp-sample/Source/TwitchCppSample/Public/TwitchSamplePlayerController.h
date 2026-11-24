#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TwitchSamplePlayerController.generated.h"

class STwitchSampleRoot;
class SWidget;
class FTwitchSampleContext;

/**
 * Adds the sample's Slate UI to the viewport when play starts and removes it when play ends.
 */
UCLASS()
class TWITCHCPPSAMPLE_API ATwitchSamplePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATwitchSamplePlayerController();

	/** The shared sample state, valid while the UI is on screen. */
	TSharedPtr<FTwitchSampleContext> GetSampleContext() const { return Context; }

	/** The root widget, valid while the UI is on screen. */
	TSharedPtr<STwitchSampleRoot> GetSampleRoot() const { return Root; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<FTwitchSampleContext> Context;
	TSharedPtr<STwitchSampleRoot> Root;
	TSharedPtr<SWidget> ViewportWidget;
};
