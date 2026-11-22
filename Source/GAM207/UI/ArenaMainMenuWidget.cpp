#include "UI/ArenaMainMenuWidget.h"
#include "Game/ArenaGameInstance.h"
#include "Gameplay/ArenaEnemyArchetype.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "ArenaMainMenu"

namespace
{
	/** Sets range, step and decimals so the Widget Blueprint doesn't have to. */
	void ConfigureSpinBox(USpinBox* SpinBox, float Min, float Max, float Step, int32 Decimals)
	{
		if (!SpinBox)
		{
			return;
		}
		SpinBox->SetMinValue(Min);
		SpinBox->SetMaxValue(Max);
		SpinBox->SetMinSliderValue(Min);
		SpinBox->SetMaxSliderValue(Max);
		SpinBox->SetDelta(Step);
		SpinBox->SetAlwaysUsesDeltaSnap(true);
		SpinBox->SetMinFractionalDigits(Decimals);
		SpinBox->SetMaxFractionalDigits(Decimals);
	}

	void SetSpin(USpinBox* SpinBox, float Value)
	{
		if (SpinBox)
		{
			SpinBox->SetValue(Value);
		}
	}

	float GetSpin(const USpinBox* SpinBox, float Fallback)
	{
		return SpinBox ? SpinBox->GetValue() : Fallback;
	}
}

UArenaGameInstance* UArenaMainMenuWidget::GetArenaGameInstance() const
{
	return GetGameInstance<UArenaGameInstance>();
}

void UArenaMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Percent inputs: 100 = normal.
	ConfigureSpinBox(EnemyCountInput, 0.f, 16.f, 1.f, 0);
	ConfigureSpinBox(EnemyHealthInput, 25.f, 400.f, 25.f, 0);
	ConfigureSpinBox(EnemyDamageInput, 25.f, 400.f, 25.f, 0);
	ConfigureSpinBox(PlayerHealthInput, 25.f, 400.f, 25.f, 0);
	ConfigureSpinBox(PlayerDamageInput, 25.f, 400.f, 25.f, 0);
	ConfigureSpinBox(PointsPerKillInput, 1.f, 10.f, 1.f, 0);
	ConfigureSpinBox(ScoreLimitInput, 0.f, 100.f, 1.f, 0);
	ConfigureSpinBox(TimeLimitInput, 0.f, 30.f, 1.f, 0);
	ConfigureSpinBox(RespawnDelayInput, 0.5f, 10.f, 0.5f, 1);

	if (PlayButton) { PlayButton->OnClicked.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandlePlay); }
	if (QuitButton) { QuitButton->OnClicked.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandleQuit); }
	if (StartMatchButton) { StartMatchButton->OnClicked.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandleStartMatch); }
	if (BackButton) { BackButton->OnClicked.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandleBack); }
	if (ResetButton) { ResetButton->OnClicked.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandleReset); }

	for (USpinBox* SpinBox : { EnemyCountInput.Get(), EnemyHealthInput.Get(), EnemyDamageInput.Get(), PlayerHealthInput.Get(), PlayerDamageInput.Get(),
		PointsPerKillInput.Get(), ScoreLimitInput.Get(), TimeLimitInput.Get(), RespawnDelayInput.Get() })
	{
		if (SpinBox)
		{
			SpinBox->OnValueChanged.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandleAnyValueChanged);
		}
	}
	if (PickupsCheckBox) { PickupsCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandlePickupsChanged); }
	if (EnemyTypeComboBox) { EnemyTypeComboBox->OnSelectionChanged.AddUniqueDynamic(this, &UArenaMainMenuWidget::HandleEnemyTypeChanged); }
}

void UArenaMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	LoadFromSettings();
	ShowMainPage();
}

void UArenaMainMenuWidget::ShowMainPage()
{
	if (MenuSwitcher)
	{
		MenuSwitcher->SetActiveWidgetIndex(0);
	}
}

void UArenaMainMenuWidget::ShowMatchSetup()
{
	LoadFromSettings();
	if (MenuSwitcher && MenuSwitcher->GetNumWidgets() > 1)
	{
		MenuSwitcher->SetActiveWidgetIndex(1);
	}
}

void UArenaMainMenuWidget::LoadFromSettings()
{
	UArenaGameInstance* GameInstance = GetArenaGameInstance();
	if (!GameInstance)
	{
		return;
	}

	TGuardValue<bool> Guard(bLoading, true);
	const FArenaMatchSettings& Settings = GameInstance->MatchSettings;

	SetSpin(EnemyCountInput, Settings.EnemyCount);
	SetSpin(EnemyHealthInput, Settings.EnemyHealthMultiplier * 100.f);
	SetSpin(EnemyDamageInput, Settings.EnemyDamageMultiplier * 100.f);
	SetSpin(PlayerHealthInput, Settings.PlayerHealthMultiplier * 100.f);
	SetSpin(PlayerDamageInput, Settings.PlayerDamageMultiplier * 100.f);
	SetSpin(PointsPerKillInput, Settings.PointsPerKill);
	SetSpin(ScoreLimitInput, Settings.ScoreLimit);
	SetSpin(TimeLimitInput, Settings.TimeLimitMinutes);
	SetSpin(RespawnDelayInput, Settings.RespawnDelay);

	if (PickupsCheckBox)
	{
		PickupsCheckBox->SetIsChecked(Settings.bPickupsEnabled);
	}

	if (EnemyTypeComboBox)
	{
		EnemyTypeComboBox->ClearOptions();
		EnemyTypeComboBox->AddOption(LOCTEXT("Mixed", "Mixed (all types)").ToString());
		for (const UArenaEnemyArchetype* Archetype : GameInstance->GetArchetypes())
		{
			EnemyTypeComboBox->AddOption(FText::Format(LOCTEXT("OnlyType", "{0} only"), Archetype->DisplayName).ToString());
		}
		const int32 Index = Settings.EnemyArchetypeIndex + 1;
		EnemyTypeComboBox->SetSelectedIndex(Index >= 0 && Index < EnemyTypeComboBox->GetOptionCount() ? Index : 0);
	}

	RefreshSummary();
}

void UArenaMainMenuWidget::SaveToSettings()
{
	UArenaGameInstance* GameInstance = GetArenaGameInstance();
	if (!GameInstance || bLoading)
	{
		return;
	}

	FArenaMatchSettings& Settings = GameInstance->MatchSettings;
	Settings.EnemyCount = FMath::RoundToInt(GetSpin(EnemyCountInput, Settings.EnemyCount));
	Settings.EnemyHealthMultiplier = GetSpin(EnemyHealthInput, Settings.EnemyHealthMultiplier * 100.f) / 100.f;
	Settings.EnemyDamageMultiplier = GetSpin(EnemyDamageInput, Settings.EnemyDamageMultiplier * 100.f) / 100.f;
	Settings.PlayerHealthMultiplier = GetSpin(PlayerHealthInput, Settings.PlayerHealthMultiplier * 100.f) / 100.f;
	Settings.PlayerDamageMultiplier = GetSpin(PlayerDamageInput, Settings.PlayerDamageMultiplier * 100.f) / 100.f;
	Settings.PointsPerKill = FMath::RoundToInt(GetSpin(PointsPerKillInput, Settings.PointsPerKill));
	Settings.ScoreLimit = FMath::RoundToInt(GetSpin(ScoreLimitInput, Settings.ScoreLimit));
	Settings.TimeLimitMinutes = GetSpin(TimeLimitInput, Settings.TimeLimitMinutes);
	Settings.RespawnDelay = GetSpin(RespawnDelayInput, Settings.RespawnDelay);
	if (PickupsCheckBox)
	{
		Settings.bPickupsEnabled = PickupsCheckBox->IsChecked();
	}
	if (EnemyTypeComboBox)
	{
		Settings.EnemyArchetypeIndex = EnemyTypeComboBox->GetSelectedIndex() - 1; // 0 = mixed -> -1
	}

	RefreshSummary();
}

void UArenaMainMenuWidget::RefreshSummary()
{
	const UArenaGameInstance* GameInstance = GetArenaGameInstance();
	if (!SummaryText || !GameInstance)
	{
		return;
	}
	const FArenaMatchSettings& Settings = GameInstance->MatchSettings;

	const FText ScoreLimit = Settings.ScoreLimit > 0
		? FText::Format(LOCTEXT("FirstTo", "first to {0}"), FText::AsNumber(Settings.ScoreLimit))
		: LOCTEXT("NoScoreLimit", "no score limit");
	const FText TimeLimit = Settings.TimeLimitMinutes > 0.f
		? FText::Format(LOCTEXT("Minutes", "{0} min"), FText::AsNumber(FMath::RoundToInt(Settings.TimeLimitMinutes)))
		: LOCTEXT("NoTimeLimit", "no time limit");

	SummaryText->SetText(FText::Format(LOCTEXT("Summary", "{0} enemies, {1}, {2}. Enemy HP {3}, your HP {4}."),
		FText::AsNumber(Settings.EnemyCount), ScoreLimit, TimeLimit,
		FText::AsPercent(Settings.EnemyHealthMultiplier), FText::AsPercent(Settings.PlayerHealthMultiplier)));
}

void UArenaMainMenuWidget::HandlePlay()
{
	if (MenuSwitcher && MenuSwitcher->GetNumWidgets() > 1)
	{
		ShowMatchSetup();
	}
	else
	{
		HandleStartMatch();
	}
}

void UArenaMainMenuWidget::HandleQuit()
{
	if (UArenaGameInstance* GameInstance = GetArenaGameInstance())
	{
		GameInstance->QuitGame();
	}
}

void UArenaMainMenuWidget::HandleStartMatch()
{
	SaveToSettings();
	if (UArenaGameInstance* GameInstance = GetArenaGameInstance())
	{
		GameInstance->StartMatch();
	}
}

void UArenaMainMenuWidget::HandleBack()
{
	ShowMainPage();
}

void UArenaMainMenuWidget::HandleReset()
{
	if (UArenaGameInstance* GameInstance = GetArenaGameInstance())
	{
		GameInstance->ResetMatchSettings();
	}
	LoadFromSettings();
}

void UArenaMainMenuWidget::HandleAnyValueChanged(float Value)
{
	SaveToSettings();
}

void UArenaMainMenuWidget::HandlePickupsChanged(bool bIsChecked)
{
	SaveToSettings();
}

void UArenaMainMenuWidget::HandleEnemyTypeChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	SaveToSettings();
}

#undef LOCTEXT_NAMESPACE
