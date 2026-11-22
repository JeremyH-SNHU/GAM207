#include "UI/ArenaScoreboardRowWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

void UArenaScoreboardRowWidget::SetRowData(int32 Rank, const FArenaCombatantStats& Stats, bool bIsLocalPlayer)
{
	const FSlateColor Color(bIsLocalPlayer ? LocalPlayerTextColor : TextColor);
	auto Fill = [&Color](UTextBlock* Text, const FText& Value)
	{
		if (Text)
		{
			Text->SetText(Value);
			Text->SetColorAndOpacity(Color);
		}
	};

	Fill(RankText, FText::AsNumber(Rank));
	Fill(NameText, Stats.DisplayName);
	Fill(KillsText, FText::AsNumber(Stats.Kills));
	Fill(DeathsText, FText::AsNumber(Stats.Deaths));
	Fill(ScoreText, FText::AsNumber(Stats.Score));
	Fill(AccuracyText, FText::AsPercent(Stats.GetAccuracy()));

	if (Background)
	{
		Background->SetBrushColor(bIsLocalPlayer ? LocalPlayerBackgroundColor : BackgroundColor);
	}

	BP_OnRowDataSet(Rank, Stats, bIsLocalPlayer);
}
