#include "Game/ArenaPlayerController.h"
#include "UI/ArenaEndMatchWidget.h"
#include "UI/ArenaHUDWidget.h"
#include "UI/ArenaScoreboardWidget.h"
#include "UI/ArenaUIHelpers.h"

AArenaPlayerController::AArenaPlayerController()
{
	HUDWidgetClass = TSoftClassPtr<UArenaHUDWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_HUD.WBP_HUD_C")));
	ScoreboardWidgetClass = TSoftClassPtr<UArenaScoreboardWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_Scoreboard.WBP_Scoreboard_C")));
	EndMatchWidgetClass = TSoftClassPtr<UArenaEndMatchWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_EndMatch.WBP_EndMatch_C")));
}

void AArenaPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController())
	{
		return;
	}

	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	HUDWidget = ArenaUI::CreateFromSoftClass(this, HUDWidgetClass, TEXT("HUD"));
	if (HUDWidget)
	{
		HUDWidget->AddToViewport(0);
	}
}

void AArenaPlayerController::SetScoreboardVisible(bool bVisible)
{
	if (bEndScreenShown || bVisible == bScoreboardShown)
	{
		return;
	}

	if (bVisible)
	{
		if (!ScoreboardWidget)
		{
			ScoreboardWidget = ArenaUI::CreateFromSoftClass(this, ScoreboardWidgetClass, TEXT("Scoreboard"));
		}
		if (ScoreboardWidget)
		{
			ScoreboardWidget->AddToViewport(10);
			ScoreboardWidget->Refresh();
			bScoreboardShown = true;
		}
	}
	else
	{
		if (ScoreboardWidget)
		{
			ScoreboardWidget->RemoveFromParent();
		}
		bScoreboardShown = false;
	}
}

bool AArenaPlayerController::ShowEndOfMatch()
{
	SetScoreboardVisible(false);
	if (HUDWidget)
	{
		HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!EndMatchWidget)
	{
		EndMatchWidget = ArenaUI::CreateFromSoftClass(this, EndMatchWidgetClass, TEXT("End of match"));
	}
	if (!EndMatchWidget)
	{
		// No end screen yet: at least show the scoreboard so the result is visible.
		SetScoreboardVisible(true);
		return false;
	}

	bEndScreenShown = true;
	EndMatchWidget->AddToViewport(100);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(EndMatchWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
	return true;
}
