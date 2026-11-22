#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaOverheadWidget.generated.h"

class AArenaCharacter;
class UProgressBar;
class UTextBlock;

/**
 * Name + health bar floating over an enemy. Reparent AI_W to this (or make a new Widget Blueprint from it)
 * and name its widgets HealthBar and NameText. The enemy's Widget Component finds it automatically.
 */
UCLASS(Abstract)
class GAM207_API UArenaOverheadWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena|UI")
	void SetCombatant(AArenaCharacter* InCombatant);

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(EditAnywhere, Category = "Arena|UI")
	FLinearColor HealthyColor = FLinearColor(0.2f, 0.85f, 0.3f);

	UPROPERTY(EditAnywhere, Category = "Arena|UI")
	FLinearColor HurtColor = FLinearColor(0.9f, 0.15f, 0.1f);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|UI", meta = (DisplayName = "On Combatant Set"))
	void BP_OnCombatantSet(AArenaCharacter* InCombatant);

private:
	UFUNCTION()
	void HandleHealthChanged(float Health, float MaxHealth);

	TWeakObjectPtr<AArenaCharacter> Combatant;
};
