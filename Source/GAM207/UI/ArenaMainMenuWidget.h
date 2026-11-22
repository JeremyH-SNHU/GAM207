#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "ArenaMainMenuWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class USpinBox;
class UTextBlock;
class UWidgetSwitcher;
class UArenaGameInstance;

/**
 * Main menu with a match-setup page. Make WBP_MainMenu in /Game/UI from this. Every widget is optional;
 * name it like the property below and the C++ wires it up (ranges, current values, saving to the Game Instance).
 *
 *  MenuSwitcher (Widget Switcher: page 0 = main, page 1 = match setup)
 *  Main page:  PlayButton, QuitButton
 *  Setup page: EnemyCountInput, EnemyHealthInput, EnemyDamageInput, PlayerHealthInput, PlayerDamageInput,
 *              PointsPerKillInput, ScoreLimitInput, TimeLimitInput, RespawnDelayInput (Spin Boxes),
 *              PickupsCheckBox (Check Box), EnemyTypeComboBox (Combo Box (String)),
 *              StartMatchButton, BackButton, ResetButton, SummaryText
 */
UCLASS(Abstract)
class GAM207_API UArenaMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena|Menu")
	void ShowMainPage();

	UFUNCTION(BlueprintCallable, Category = "Arena|Menu")
	void ShowMatchSetup();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidgetSwitcher> MenuSwitcher;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PlayButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> EnemyCountInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> EnemyHealthInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> EnemyDamageInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> PlayerHealthInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> PlayerDamageInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> PointsPerKillInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> ScoreLimitInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> TimeLimitInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<USpinBox> RespawnDelayInput;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCheckBox> PickupsCheckBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UComboBoxString> EnemyTypeComboBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> StartMatchButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> BackButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ResetButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> SummaryText;

private:
	UFUNCTION() void HandlePlay();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleStartMatch();
	UFUNCTION() void HandleBack();
	UFUNCTION() void HandleReset();
	UFUNCTION() void HandleAnyValueChanged(float Value);
	UFUNCTION() void HandlePickupsChanged(bool bIsChecked);
	UFUNCTION() void HandleEnemyTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UArenaGameInstance* GetArenaGameInstance() const;
	void LoadFromSettings();
	void SaveToSettings();
	void RefreshSummary();

	bool bLoading = false;
};
