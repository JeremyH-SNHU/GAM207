#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ArenaPlayerController.generated.h"

class UArenaHUDWidget;
class UArenaScoreboardWidget;
class UArenaEndMatchWidget;

/**
 * In-match player controller. Reparent PlayerController_BP to this.
 * Creates the HUD, shows the scoreboard while Tab is held, and shows the end-of-match screen.
 * Widget classes default to /Game/UI/WBP_HUD, WBP_Scoreboard and WBP_EndMatch.
 */
UCLASS()
class GAM207_API AArenaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AArenaPlayerController();

	UFUNCTION(BlueprintCallable, Category = "Arena|UI")
	void SetScoreboardVisible(bool bVisible);

	/** Returns false if the end screen widget couldn't be created. */
	bool ShowEndOfMatch();

	UFUNCTION(BlueprintPure, Category = "Arena|UI")
	UArenaHUDWidget* GetHUDWidget() const { return HUDWidget; }

	UPROPERTY(EditDefaultsOnly, Category = "Arena|UI")
	TSoftClassPtr<UArenaHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|UI")
	TSoftClassPtr<UArenaScoreboardWidget> ScoreboardWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|UI")
	TSoftClassPtr<UArenaEndMatchWidget> EndMatchWidgetClass;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UArenaHUDWidget> HUDWidget;

	UPROPERTY(Transient)
	TObjectPtr<UArenaScoreboardWidget> ScoreboardWidget;

	UPROPERTY(Transient)
	TObjectPtr<UArenaEndMatchWidget> EndMatchWidget;

	bool bScoreboardShown = false;
	bool bEndScreenShown = false;
};
