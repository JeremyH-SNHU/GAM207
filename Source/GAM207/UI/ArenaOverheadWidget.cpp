#include "UI/ArenaOverheadWidget.h"
#include "Characters/ArenaCharacter.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UArenaOverheadWidget::SetCombatant(AArenaCharacter* InCombatant)
{
	if (AArenaCharacter* Previous = Combatant.Get())
	{
		Previous->OnHealthChanged.RemoveDynamic(this, &UArenaOverheadWidget::HandleHealthChanged);
	}

	Combatant = InCombatant;
	if (!InCombatant)
	{
		return;
	}

	InCombatant->OnHealthChanged.AddUniqueDynamic(this, &UArenaOverheadWidget::HandleHealthChanged);
	if (NameText)
	{
		NameText->SetText(InCombatant->GetCombatantName());
	}
	HandleHealthChanged(InCombatant->GetHealth(), InCombatant->GetMaxHealth());
	BP_OnCombatantSet(InCombatant);
}

void UArenaOverheadWidget::NativeDestruct()
{
	if (AArenaCharacter* Current = Combatant.Get())
	{
		Current->OnHealthChanged.RemoveDynamic(this, &UArenaOverheadWidget::HandleHealthChanged);
	}
	Super::NativeDestruct();
}

void UArenaOverheadWidget::HandleHealthChanged(float Health, float MaxHealth)
{
	if (HealthBar)
	{
		const float Fraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
		HealthBar->SetPercent(Fraction);
		HealthBar->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(HurtColor, HealthyColor, Fraction));
	}
}
