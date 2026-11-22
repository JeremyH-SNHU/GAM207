#pragma once

#include "CoreMinimal.h"
#include "UI/ArenaScoreboardWidget.h"
#include "ArenaEndMatchWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * End-of-match screen: winner banner, final scoreboard and your stats. Make WBP_EndMatch in /Game/UI from this.
 * Inherits RowsBox / Row Class from the scoreboard. Optional: WinnerText, ReasonText, PlayerSummaryText,
 * and Buttons named PlayAgainButton, MatchSettingsButton, MainMenuButton, QuitButton.
 */
UCLASS(Abstract)
class GAM207_API UArenaEndMatchWidget : public UArenaScoreboardWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> WinnerText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ReasonText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> PlayerSummaryText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PlayAgainButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MatchSettingsButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> MainMenuButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> QuitButton;

	UPROPERTY(EditAnywhere, Category = "Arena|End Screen") FLinearColor VictoryColor = FLinearColor(1.f, 0.8f, 0.2f);
	UPROPERTY(EditAnywhere, Category = "Arena|End Screen") FLinearColor DefeatColor = FLinearColor(0.9f, 0.3f, 0.25f);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|End Screen", meta = (DisplayName = "On Match Result"))
	void BP_OnMatchResult(bool bPlayerWon);

private:
	UFUNCTION() void HandlePlayAgain();
	UFUNCTION() void HandleMatchSettings();
	UFUNCTION() void HandleMainMenu();
	UFUNCTION() void HandleQuit();
};
