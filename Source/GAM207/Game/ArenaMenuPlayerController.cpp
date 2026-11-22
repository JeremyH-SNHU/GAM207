#include "Game/ArenaMenuPlayerController.h"
#include "Game/ArenaGameInstance.h"
#include "UI/ArenaMainMenuWidget.h"
#include "UI/ArenaUIHelpers.h"
#include "Engine/Engine.h"

AArenaMenuPlayerController::AArenaMenuPlayerController()
{
	MainMenuWidgetClass = TSoftClassPtr<UArenaMainMenuWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_MainMenu.WBP_MainMenu_C")));
}

void AArenaMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	MenuWidget = ArenaUI::CreateFromSoftClass(this, MainMenuWidgetClass, TEXT("Main menu"));
	if (!MenuWidget)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 30.f, FColor::Red, TEXT("WBP_MainMenu not found. Create it in /Game/UI with parent class ArenaMainMenuWidget."));
		}
		return;
	}

	MenuWidget->AddToViewport();

	if (UArenaGameInstance* GameInstance = GetGameInstance<UArenaGameInstance>())
	{
		if (GameInstance->bOpenMatchSetupOnMenuLoad)
		{
			MenuWidget->ShowMatchSetup();
			GameInstance->bOpenMatchSetupOnMenuLoad = false;
		}
	}

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(MenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}
