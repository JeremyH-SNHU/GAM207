#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaTypes.h"
#include "ArenaScoreboardRowWidget.generated.h"

class UBorder;
class UTextBlock;

/**
 * One scoreboard line. Reparent ArenaPlayers to this (or make WBP_ScoreRow from it).
 * Optional widgets: RankText, NameText, KillsText, DeathsText, ScoreText, AccuracyText, and a Border named Background.
 */
UCLASS(Abstract)
class GAM207_API UArenaScoreboardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena|Scoreboard")
	void SetRowData(int32 Rank, const FArenaCombatantStats& Stats, bool bIsLocalPlayer);

protected:
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> RankText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> KillsText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> DeathsText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AccuracyText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UBorder> Background;

	UPROPERTY(EditAnywhere, Category = "Arena|Scoreboard") FLinearColor TextColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category = "Arena|Scoreboard") FLinearColor LocalPlayerTextColor = FLinearColor(1.f, 0.85f, 0.25f);
	UPROPERTY(EditAnywhere, Category = "Arena|Scoreboard") FLinearColor BackgroundColor = FLinearColor(0.f, 0.f, 0.f, 0.35f);
	UPROPERTY(EditAnywhere, Category = "Arena|Scoreboard") FLinearColor LocalPlayerBackgroundColor = FLinearColor(0.35f, 0.25f, 0.f, 0.55f);

	/** Runs after the texts are filled in, for any extra styling. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Scoreboard", meta = (DisplayName = "On Row Data Set"))
	void BP_OnRowDataSet(int32 Rank, const FArenaCombatantStats& Stats, bool bIsLocalPlayer);
};
