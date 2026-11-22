#include "UI/ArenaHUDWidget.h"
#include "Characters/ArenaCharacter.h"
#include "Game/ArenaGameMode.h"
#include "UI/ArenaUIHelpers.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "ArenaHUD"

void UArenaHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HitMarker) { HitMarker->SetVisibility(ESlateVisibility::Hidden); }
	if (DamageFlash) { DamageFlash->SetRenderOpacity(0.f); DamageFlash->SetVisibility(ESlateVisibility::HitTestInvisible); }
	if (RespawnText) { RespawnText->SetVisibility(ESlateVisibility::Collapsed); }
	if (AnnouncementText) { AnnouncementText->SetVisibility(ESlateVisibility::Collapsed); }
	if (KillFeedBox) { KillFeedBox->ClearChildren(); }

	if (AArenaGameMode* GameMode = GetArenaGameMode())
	{
		BoundGameMode = GameMode;
		GameMode->OnKill.AddUniqueDynamic(this, &UArenaHUDWidget::HandleKill);
		GameMode->OnScoresChanged.AddUniqueDynamic(this, &UArenaHUDWidget::HandleScoresChanged);
	}

	BindCharacter(Cast<AArenaCharacter>(GetOwningPlayerPawn()));
	RefreshScore();
	RefreshTickingText();
}

void UArenaHUDWidget::NativeDestruct()
{
	UnbindCharacter();
	if (AArenaGameMode* GameMode = BoundGameMode.Get())
	{
		GameMode->OnKill.RemoveDynamic(this, &UArenaHUDWidget::HandleKill);
		GameMode->OnScoresChanged.RemoveDynamic(this, &UArenaHUDWidget::HandleScoresChanged);
	}
	Super::NativeDestruct();
}

AArenaGameMode* UArenaHUDWidget::GetArenaGameMode() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetAuthGameMode<AArenaGameMode>() : nullptr;
}

void UArenaHUDWidget::BindCharacter(AArenaCharacter* Character)
{
	if (Character == BoundCharacter.Get())
	{
		return;
	}
	UnbindCharacter();
	BoundCharacter = Character;
	if (!Character)
	{
		return;
	}

	Character->OnHealthChanged.AddUniqueDynamic(this, &UArenaHUDWidget::HandleHealthChanged);
	Character->OnDamaged.AddUniqueDynamic(this, &UArenaHUDWidget::HandleDamaged);
	Character->OnHitLanded.AddUniqueDynamic(this, &UArenaHUDWidget::HandleHitLanded);
	HandleHealthChanged(Character->GetHealth(), Character->GetMaxHealth());
}

void UArenaHUDWidget::UnbindCharacter()
{
	if (AArenaCharacter* Character = BoundCharacter.Get())
	{
		Character->OnHealthChanged.RemoveDynamic(this, &UArenaHUDWidget::HandleHealthChanged);
		Character->OnDamaged.RemoveDynamic(this, &UArenaHUDWidget::HandleDamaged);
		Character->OnHitLanded.RemoveDynamic(this, &UArenaHUDWidget::HandleHitLanded);
	}
	BoundCharacter = nullptr;
}

void UArenaHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UITime += InDeltaTime;

	// The pawn can arrive after the HUD is created; pick it up whenever it changes.
	AArenaCharacter* Pawn = Cast<AArenaCharacter>(GetOwningPlayerPawn());
	if (Pawn != BoundCharacter.Get())
	{
		BindCharacter(Pawn);
		RefreshScore();
	}

	if (HitMarker && !HitMarkerAnim && HitMarkerHideTime > 0.f && UITime >= HitMarkerHideTime)
	{
		HitMarker->SetVisibility(ESlateVisibility::Hidden);
		HitMarkerHideTime = 0.f;
	}

	if (AnnouncementText && AnnouncementHideTime > 0.f && UITime >= AnnouncementHideTime)
	{
		AnnouncementText->SetVisibility(ESlateVisibility::Collapsed);
		AnnouncementHideTime = 0.f;
	}

	if (DamageFlash && !DamageFlashAnim && DamageFlashOpacity > 0.f)
	{
		DamageFlashOpacity = FMath::Max(0.f, DamageFlashOpacity - DamageFlashFadePerSecond * InDeltaTime);
		DamageFlash->SetRenderOpacity(DamageFlashOpacity);
	}

	for (int32 Index = KillFeedItems.Num() - 1; Index >= 0; --Index)
	{
		if (UITime >= KillFeedItems[Index].ExpireTime)
		{
			if (UWidget* Entry = KillFeedItems[Index].Widget.Get())
			{
				Entry->RemoveFromParent();
			}
			KillFeedItems.RemoveAt(Index);
		}
	}

	RefreshTickingText();
}

// ==================================================================== Event handlers

void UArenaHUDWidget::HandleHealthChanged(float Health, float MaxHealth)
{
	const float Fraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (HealthBar)
	{
		HealthBar->SetPercent(Fraction);
		HealthBar->SetFillColorAndOpacity(FLinearColor::LerpUsingHSV(HurtColor, HealthyColor, Fraction));
	}
	if (HealthText)
	{
		HealthText->SetText(FText::Format(LOCTEXT("HealthFormat", "{0} / {1}"), FText::AsNumber(FMath::CeilToInt(Health)), FText::AsNumber(FMath::CeilToInt(MaxHealth))));
	}
}

void UArenaHUDWidget::HandleDamaged(AArenaCharacter* Attacker, float Damage, FVector HitDirection)
{
	if (DamageFlashAnim)
	{
		PlayAnimation(DamageFlashAnim);
	}
	else if (DamageFlash)
	{
		DamageFlashOpacity = FMath::Min(DamageFlashMaxOpacity, DamageFlashOpacity + DamageFlashMaxOpacity * 0.6f);
		DamageFlash->SetRenderOpacity(DamageFlashOpacity);
	}
	BP_OnPlayerDamaged(Damage, HitDirection);
}

void UArenaHUDWidget::HandleHitLanded(AArenaCharacter* Victim, float Damage, bool bKilled)
{
	if (HitMarkerAnim)
	{
		PlayAnimation(HitMarkerAnim);
	}
	else if (HitMarker)
	{
		HitMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
		HitMarker->SetRenderScale(bKilled ? FVector2D(1.6f, 1.6f) : FVector2D(1.f, 1.f));
		HitMarkerHideTime = UITime + (bKilled ? HitMarkerDuration * 2.f : HitMarkerDuration);
	}
	BP_OnHitMarker(bKilled);
}

void UArenaHUDWidget::HandleKill(AArenaCharacter* Killer, AArenaCharacter* Victim, int32 PointsAwarded)
{
	const AArenaCharacter* Me = BoundCharacter.Get();
	const FText VictimName = Victim ? Victim->GetCombatantName() : FText::GetEmpty();

	FText Line;
	if (Killer && Killer != Victim)
	{
		Line = FText::Format(LOCTEXT("KillFeedFormat", "{0}  eliminated  {1}"), Killer->GetCombatantName(), VictimName);
	}
	else
	{
		Line = FText::Format(LOCTEXT("DiedFormat", "{0}  died"), VictimName);
	}

	FLinearColor Color = OtherKillColor;
	if (Me && Killer == Me)
	{
		Color = PlayerKillColor;
		ShowAnnouncement(FText::Format(LOCTEXT("YouEliminated", "ELIMINATED {0}   +{1}"), VictimName, FText::AsNumber(PointsAwarded)), PlayerKillColor);
	}
	else if (Me && Victim == Me)
	{
		Color = PlayerDeathColor;
		if (Killer && Killer != Me)
		{
			ShowAnnouncement(FText::Format(LOCTEXT("EliminatedBy", "Eliminated by {0}"), Killer->GetCombatantName()), PlayerDeathColor);
		}
	}

	AddKillFeedEntry(Line, Color);
	BP_OnKill(Killer, Victim);
}

void UArenaHUDWidget::HandleScoresChanged()
{
	RefreshScore();
}

// ==================================================================== Display

void UArenaHUDWidget::AddKillFeedEntry(const FText& Text, FLinearColor Color)
{
	if (!KillFeedBox || !WidgetTree)
	{
		return;
	}

	UTextBlock* Entry = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	FSlateFontInfo Font = Entry->GetFont();
	Font.Size = KillFeedFontSize;
	Entry->SetFont(Font);
	Entry->SetText(Text);
	Entry->SetColorAndOpacity(FSlateColor(Color));
	Entry->SetShadowOffset(FVector2D(1.f, 1.f));
	Entry->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	KillFeedBox->AddChild(Entry);

	FKillFeedItem& Item = KillFeedItems.AddDefaulted_GetRef();
	Item.Widget = Entry;
	Item.ExpireTime = UITime + KillFeedLifetime;

	while (KillFeedItems.Num() > MaxKillFeedEntries)
	{
		if (UWidget* Oldest = KillFeedItems[0].Widget.Get())
		{
			Oldest->RemoveFromParent();
		}
		KillFeedItems.RemoveAt(0);
	}
}

void UArenaHUDWidget::ShowAnnouncement(const FText& Text, FLinearColor Color, float Duration)
{
	if (!AnnouncementText)
	{
		return;
	}
	AnnouncementText->SetText(Text);
	AnnouncementText->SetColorAndOpacity(FSlateColor(Color));
	AnnouncementText->SetVisibility(ESlateVisibility::HitTestInvisible);
	AnnouncementHideTime = UITime + Duration;
}

void UArenaHUDWidget::RefreshScore()
{
	const AArenaGameMode* GameMode = BoundGameMode.Get();
	const AArenaCharacter* Me = BoundCharacter.Get();
	if (!GameMode)
	{
		return;
	}

	const FArenaMatchSettings Settings = GameMode->GetMatchSettings();
	const TArray<FArenaCombatantStats> Sorted = GameMode->GetSortedStats();

	FArenaCombatantStats MyStats;
	const bool bHaveMyStats = Me && GameMode->GetStatsFor(Me, MyStats);

	if (ScoreText)
	{
		const int32 Score = bHaveMyStats ? MyStats.Score : 0;
		ScoreText->SetText(Settings.ScoreLimit > 0
			? FText::Format(LOCTEXT("ScoreWithLimit", "SCORE  {0} / {1}"), FText::AsNumber(Score), FText::AsNumber(Settings.ScoreLimit))
			: FText::Format(LOCTEXT("Score", "SCORE  {0}"), FText::AsNumber(Score)));
	}

	if (RankText)
	{
		const int32 Rank = Me ? GameMode->GetRankOf(Me) : 0;
		RankText->SetText(Rank > 0
			? FText::Format(LOCTEXT("Rank", "#{0} of {1}"), FText::AsNumber(Rank), FText::AsNumber(Sorted.Num()))
			: FText::GetEmpty());
	}

	if (LeaderText)
	{
		if (Sorted.Num() == 0)
		{
			LeaderText->SetText(FText::GetEmpty());
		}
		else if (Sorted[0].Combatant == Me)
		{
			LeaderText->SetText(LOCTEXT("YouLead", "You're in the lead"));
		}
		else
		{
			LeaderText->SetText(FText::Format(LOCTEXT("Leader", "Leader: {0} ({1})"), Sorted[0].DisplayName, FText::AsNumber(Sorted[0].Score)));
		}
	}
}

void UArenaHUDWidget::RefreshTickingText()
{
	if (TimerText)
	{
		const AArenaGameMode* GameMode = BoundGameMode.Get();
		const float Remaining = GameMode ? GameMode->GetTimeRemaining() : -1.f;
		if (Remaining < 0.f)
		{
			TimerText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			TimerText->SetVisibility(ESlateVisibility::HitTestInvisible);
			TimerText->SetText(ArenaUI::FormatTime(Remaining));
		}
	}

	const AArenaCharacter* Me = BoundCharacter.Get();

	if (BuffText)
	{
		TArray<FString> Lines;
		if (Me && Me->GetDamageBoostTimeRemaining() > 0.f)
		{
			Lines.Add(FString::Printf(TEXT("DAMAGE x%.1f   %.0fs"), Me->GetDamageBoostMultiplier(), FMath::CeilToFloat(Me->GetDamageBoostTimeRemaining())));
		}
		if (Me && Me->GetSpeedBoostTimeRemaining() > 0.f)
		{
			Lines.Add(FString::Printf(TEXT("SPEED x%.1f   %.0fs"), Me->GetSpeedBoostMultiplier(), FMath::CeilToFloat(Me->GetSpeedBoostTimeRemaining())));
		}
		BuffText->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
	}

	if (RespawnText)
	{
		if (Me && !Me->IsAlive())
		{
			RespawnText->SetVisibility(ESlateVisibility::HitTestInvisible);
			RespawnText->SetText(FText::Format(LOCTEXT("Respawn", "Respawning in {0}"), FText::AsNumber(FMath::CeilToInt(Me->GetRespawnTimeRemaining()))));
		}
		else
		{
			RespawnText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

#undef LOCTEXT_NAMESPACE
