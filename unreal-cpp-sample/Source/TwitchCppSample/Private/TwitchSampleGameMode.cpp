#include "TwitchSampleGameMode.h"

#include "TwitchSamplePlayerController.h"

ATwitchSampleGameMode::ATwitchSampleGameMode()
{
	PlayerControllerClass = ATwitchSamplePlayerController::StaticClass();

	// The sample is UI only, so there is nothing to possess.
	DefaultPawnClass = nullptr;
}
