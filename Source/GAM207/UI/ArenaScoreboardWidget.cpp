#include "UI/ArenaScoreboardWidget.h"
#include "Characters/ArenaCharacter.h"
#include "Game/ArenaGameMode.h"
#include "UI/ArenaScoreboardRowWidget.h"
#include "UI/ArenaUIHelpers.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "ArenaScoreboard"

void UArenaScoreboardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AArenaGameMode* GameMode = GetArenaGameMode())
	{
		BoundGameMode = GameMode;
		GameMode->OnScoresChanged.AddUniqueDynamic(this, &UArenaScoreboardWidget::HandleScoresChanged);
	}
	Refresh();
}

void UArenaScoreboardWidget::NativeDestruct()
{
	if (AArenaGameMode* GameMode = BoundGameMode.Get())
	{
		GameMode->OnScoresChanged.RemoveDynamic(this, &UArenaScoreboardWidget::HandleScoresChanged);
	}
	Super::NativeDestruct();
}

AArenaGameMode* UArenaScoreboardWidget::GetArenaGameMode() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetAuthGameMode<AArenaGameMode>() : nullptr;
}

void UArenaScoreboardWidget::HandleScoresChanged()
{
	Refresh();
}

void UArenaScoreboardWidget::Refresh()
{
	const AArenaGameMode* GameMode = GetArenaGameMode();
	if (!GameMode)
	{
		return;
	}

	const FArenaMatchSettings Settings = GameMode->GetMatchSettings();
	if (TitleText)
	{
		TitleText->SetText(LOCTEXT("Title", "SCOREBOARD"));
	}
	if (MatchInfoText)
	{
		TArray<FString> Parts;
		if (Settings.ScoreLimit > 0)
		{
			Parts.Add(FString::Printf(TEXT("First to %d"), Settings.ScoreLimit));
		}
		const float Remaining = GameMode->GetTimeRemaining();
		if (Remaining >= 0.f)
		{
			Parts.Add(FString::Printf(TEXT("%s left"), *ArenaUI::FormatTime(Remaining).ToString()));
		}
		Parts.Add(FString::Printf(TEXT("%d pt%s per kill"), Settings.PointsPerKill, Settings.PointsPerKill == 1 ? TEXT("") : TEXT("s")));
		MatchInfoText->SetText(FText::FromString(FString::Join(Parts, TEXT("   |   "))));
	}

	if (!RowsBox)
	{
		return;
	}
	RowsBox->ClearChildren();

	const APawn* LocalPawn = GetOwningPlayerPawn();
	const TArray<FArenaCombatantStats> Sorted = GameMode->GetSortedStats();
	for (int32 Index = 0; Index < Sorted.Num(); ++Index)
	{
		const FArenaCombatantStats& Stats = Sorted[Index];
		const bool bIsLocal = Stats.Combatant && Stats.Combatant == LocalPawn;

		if (RowClass)
		{
			if (UArenaScoreboardRowWidget* Row = CreateWidget<UArenaScoreboardRowWidget>(GetOwningPlayer(), RowClass))
			{
				Row->SetRowData(Index + 1, Stats, bIsLocal);
				RowsBox->AddChild(Row);
			}
			continue;
		}

		// Fallback when no row widget is set: one line of text per combatant.
		UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FSlateFontInfo Font = Line->GetFont();
		Font.Size = FallbackRowFontSize;
		Line->SetFont(Font);
		Line->SetText(FText::Format(LOCTEXT("FallbackRow", "{0}.  {1}     K {2}   D {3}   Score {4}"),
			FText::AsNumber(Index + 1), Stats.DisplayName, FText::AsNumber(Stats.Kills), FText::AsNumber(Stats.Deaths), FText::AsNumber(Stats.Score)));
		Line->SetColorAndOpacity(FSlateColor(bIsLocal ? FLinearColor(1.f, 0.85f, 0.25f) : FLinearColor::White));
		RowsBox->AddChild(Line);
	}
}

#undef LOCTEXT_NAMESPACE
