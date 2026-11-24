#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TwitchSampleGameMode.generated.h"

/**
 * Game mode for the sample. It only swaps in the sample's player controller, which builds the UI.
 * Set as the project's default game mode in Config/DefaultEngine.ini.
 */
UCLASS()
class TWITCHCPPSAMPLE_API ATwitchSampleGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATwitchSampleGameMode();
};
