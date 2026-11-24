#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

#include "UI/TwitchSampleContext.h"

/**
 * Base class for one tab of the sample.
 */
class SSamplePage : public SCompoundWidget
{
public:
	virtual FString GetTitle() const = 0;

	/** Called before play ends: stop polls, predictions, and event streams created by this page. */
	virtual void ReleaseApiResources() {}

protected:
	/** A warning shown at the top of the page until the user has applied a Client ID and logged in. */
	TSharedRef<SWidget> MakeConnectionHint();

	TSharedPtr<FTwitchSampleContext> Context;
};
