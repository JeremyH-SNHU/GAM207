#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArenaMenuPlayerController.generated.h"

class UArenaMainMenuWidget;

/** Shows the main menu (default /Game/UI/WBP_MainMenu) with the mouse cursor on. */
UCLASS()
class GAM207_API AArenaMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArenaMenuPlayerController();

	UPROPERTY(EditDefaultsOnly, Category = "Arena|UI")
	TSoftClassPtr<UArenaMainMenuWidget> MainMenuWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UArenaMainMenuWidget> MenuWidget;
};
