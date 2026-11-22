#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaScoreboardWidget.generated.h"

class AArenaGameMode;
class UArenaScoreboardRowWidget;
class UPanelWidget;
class UTextBlock;

/**
 * Sorted scoreboard (shown while Tab is held). Make WBP_Scoreboard in /Game/UI from this, or reparent Score_W.
 * Needs a Vertical Box named RowsBox. Set Row Class to your row widget; without one, plain text rows are used.
 * Optional: TitleText, MatchInfoText.
 */
UCLASS(Abstract)
class GAM207_API UArenaScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena|Scoreboard")
	void Refresh();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	AArenaGameMode* GetArenaGameMode() const;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> RowsBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> MatchInfoText;

	UPROPERTY(EditAnywhere, Category = "Arena|Scoreboard")
	TSubclassOf<UArenaScoreboardRowWidget> RowClass;

	UPROPERTY(EditAnywhere, Category = "Arena|Scoreboard", meta = (ClampMin = "6"))
	int32 FallbackRowFontSize = 16;

private:
	UFUNCTION()
	void HandleScoresChanged();

	TWeakObjectPtr<AArenaGameMode> BoundGameMode;
};
