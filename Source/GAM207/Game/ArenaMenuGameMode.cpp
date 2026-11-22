#include "Game/ArenaMenuGameMode.h"
#include "Game/ArenaMenuPlayerController.h"

AArenaMenuGameMode::AArenaMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AArenaMenuPlayerController::StaticClass();
}
