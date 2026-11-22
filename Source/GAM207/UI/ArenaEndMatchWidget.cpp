#include "UI/ArenaEndMatchWidget.h"
#include "Characters/ArenaCharacter.h"
#include "Game/ArenaGameInstance.h"
#include "Game/ArenaGameMode.h"
#include "UI/ArenaUIHelpers.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "ArenaEndMatch"

void UArenaEndMatchWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (PlayAgainButton) { PlayAgainButton->OnClicked.AddUniqueDynamic(this, &UArenaEndMatchWidget::HandlePlayAgain); }
	if (MatchSettingsButton) { MatchSettingsButton->OnClicked.AddUniqueDynamic(this, &UArenaEndMatchWidget::HandleMatchSettings); }
	if (MainMenuButton) { MainMenuButton->OnClicked.AddUniqueDynamic(this, &UArenaEndMatchWidget::HandleMainMenu); }
	if (QuitButton) { QuitButton->OnClicked.AddUniqueDynamic(this, &UArenaEndMatchWidget::HandleQuit); }

	const AArenaGameMode* GameMode = GetArenaGameMode();
	if (!GameMode)
	{
		return;
	}

	const AArenaCharacter* Winner = GameMode->GetWinner();
	const APawn* LocalPawn = GetOwningPlayerPawn();
	const bool bPlayerWon = Winner && Winner == LocalPawn;

	if (WinnerText)
	{
		WinnerText->SetText(bPlayerWon
			? LOCTEXT("Victory", "VICTORY")
			: FText::Format(LOCTEXT("WinnerIs", "{0} WINS"), Winner ? Winner->GetCombatantName() : LOCTEXT("Nobody", "NOBODY")));
		WinnerText->SetColorAndOpacity(FSlateColor(bPlayerWon ? VictoryColor : DefeatColor));
	}

	if (ReasonText)
	{
		const FArenaMatchSettings Settings = GameMode->GetMatchSettings();
		ReasonText->SetText(GameMode->GetEndReason() == EArenaMatchEndReason::ScoreLimit
			? FText::Format(LOCTEXT("ScoreLimitReached", "Score limit of {0} reached"), FText::AsNumber(Settings.ScoreLimit))
			: LOCTEXT("TimeUp", "Time's up"));
	}

	if (PlayerSummaryText)
	{
		const AArenaCharacter* Me = Cast<AArenaCharacter>(LocalPawn);
		FArenaCombatantStats Stats;
		if (Me && GameMode->GetStatsFor(Me, Stats))
		{
			const float KillDeath = Stats.Deaths > 0 ? static_cast<float>(Stats.Kills) / Stats.Deaths : static_cast<float>(Stats.Kills);
			FNumberFormattingOptions OneDecimal;
			OneDecimal.SetMinimumFractionalDigits(1).SetMaximumFractionalDigits(1);
			PlayerSummaryText->SetText(FText::Format(
				LOCTEXT("Summary", "Place #{0}   |   {1} kills   {2} deaths   K/D {3}   |   Accuracy {4}   |   Damage {5}   |   Match length {6}"),
				FText::AsNumber(GameMode->GetRankOf(Me)), FText::AsNumber(Stats.Kills), FText::AsNumber(Stats.Deaths),
				FText::AsNumber(KillDeath, &OneDecimal), FText::AsPercent(Stats.GetAccuracy()),
				FText::AsNumber(FMath::RoundToInt(Stats.DamageDealt)), ArenaUI::FormatTime(GameMode->GetMatchElapsedTime())));
		}
	}

	BP_OnMatchResult(bPlayerWon);
}

void UArenaEndMatchWidget::HandlePlayAgain()
{
	if (UArenaGameInstance* GameInstance = GetGameInstance<UArenaGameInstance>())
	{
		GameInstance->StartMatch();
	}
}

void UArenaEndMatchWidget::HandleMatchSettings()
{
	if (UArenaGameInstance* GameInstance = GetGameInstance<UArenaGameInstance>())
	{
		GameInstance->OpenMainMenu(true);
	}
}

void UArenaEndMatchWidget::HandleMainMenu()
{
	if (UArenaGameInstance* GameInstance = GetGameInstance<UArenaGameInstance>())
	{
		GameInstance->OpenMainMenu(false);
	}
}

void UArenaEndMatchWidget::HandleQuit()
{
	if (UArenaGameInstance* GameInstance = GetGameInstance<UArenaGameInstance>())
	{
		GameInstance->QuitGame();
	}
}

#undef LOCTEXT_NAMESPACE
