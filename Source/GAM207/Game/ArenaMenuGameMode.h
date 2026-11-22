#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaMenuGameMode.generated.h"

/** Game mode for the main menu map: no pawn, just the menu. Set it as the MainMenu map's World Settings > GameMode Override. */
UCLASS()
class GAM207_API AArenaMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaMenuGameMode();

	virtual bool PlayerCanRestart_Implementation(APlayerController* Player) override { return false; }
};
