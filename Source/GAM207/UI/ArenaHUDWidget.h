#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArenaHUDWidget.generated.h"

class AArenaCharacter;
class AArenaGameMode;
class UPanelWidget;
class UProgressBar;
class UTextBlock;
class UWidgetAnimation;

/**
 * In-match HUD. Make WBP_HUD in /Game/UI with this as its parent class and lay it out however you like.
 * Every widget below is optional: name a widget exactly like the property and the C++ fills it in.
 *
 *   HealthBar (Progress Bar)   HealthText   ScoreText   RankText   LeaderText   TimerText   BuffText
 *   RespawnText   AnnouncementText (Text Blocks)   KillFeedBox (Vertical Box)
 *   HitMarker (any widget, e.g. an Image over the crosshair)   DamageFlash (any widget, e.g. a full-screen red Image)
 *   Optional animations: HitMarkerAnim, DamageFlashAnim (otherwise visibility/opacity are driven from code).
 */
UCLASS(Abstract)
class GAM207_API UArenaHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Arena|HUD")
	void AddKillFeedEntry(const FText& Text, FLinearColor Color);

	UFUNCTION(BlueprintCallable, Category = "Arena|HUD")
	void ShowAnnouncement(const FText& Text, FLinearColor Color, float Duration = 2.f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ScoreText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> RankText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> LeaderText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> TimerText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> BuffText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> RespawnText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> AnnouncementText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UPanelWidget> KillFeedBox;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> HitMarker;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> DamageFlash;

	UPROPERTY(Transient, meta = (BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> HitMarkerAnim;
	UPROPERTY(Transient, meta = (BindWidgetAnimOptional)) TObjectPtr<UWidgetAnimation> DamageFlashAnim;

	UPROPERTY(EditAnywhere, Category = "Arena|HUD") FLinearColor HealthyColor = FLinearColor(0.2f, 0.85f, 0.3f);
	UPROPERTY(EditAnywhere, Category = "Arena|HUD") FLinearColor HurtColor = FLinearColor(0.9f, 0.15f, 0.1f);
	UPROPERTY(EditAnywhere, Category = "Arena|HUD") FLinearColor PlayerKillColor = FLinearColor(1.f, 0.8f, 0.2f);
	UPROPERTY(EditAnywhere, Category = "Arena|HUD") FLinearColor PlayerDeathColor = FLinearColor(1.f, 0.35f, 0.3f);
	UPROPERTY(EditAnywhere, Category = "Arena|HUD") FLinearColor OtherKillColor = FLinearColor(0.85f, 0.85f, 0.85f);
	UPROPERTY(EditAnywhere, Category = "Arena|HUD", meta = (ClampMin = "6")) int32 KillFeedFontSize = 14;
	UPROPERTY(EditAnywhere, Category = "Arena|HUD", meta = (ClampMin = "0.5")) float KillFeedLifetime = 5.f;
	UPROPERTY(EditAnywhere, Category = "Arena|HUD", meta = (ClampMin = "1")) int32 MaxKillFeedEntries = 6;
	UPROPERTY(EditAnywhere, Category = "Arena|HUD", meta = (ClampMin = "0.01")) float HitMarkerDuration = 0.15f;
	UPROPERTY(EditAnywhere, Category = "Arena|HUD", meta = (ClampMin = "0", ClampMax = "1")) float DamageFlashMaxOpacity = 0.55f;
	UPROPERTY(EditAnywhere, Category = "Arena|HUD", meta = (ClampMin = "0.1")) float DamageFlashFadePerSecond = 1.5f;

	/** Extra hooks for effects you want to add in the Widget Blueprint (sounds, shakes, animations). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|HUD", meta = (DisplayName = "On Hit Marker"))
	void BP_OnHitMarker(bool bKill);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|HUD", meta = (DisplayName = "On Player Damaged"))
	void BP_OnPlayerDamaged(float Damage, FVector HitDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|HUD", meta = (DisplayName = "On Kill Feed"))
	void BP_OnKill(AArenaCharacter* Killer, AArenaCharacter* Victim);

private:
	UFUNCTION() void HandleHealthChanged(float Health, float MaxHealth);
	UFUNCTION() void HandleDamaged(AArenaCharacter* Attacker, float Damage, FVector HitDirection);
	UFUNCTION() void HandleHitLanded(AArenaCharacter* Victim, float Damage, bool bKilled);
	UFUNCTION() void HandleKill(AArenaCharacter* Killer, AArenaCharacter* Victim, int32 PointsAwarded);
	UFUNCTION() void HandleScoresChanged();

	void BindCharacter(AArenaCharacter* Character);
	void UnbindCharacter();
	AArenaGameMode* GetArenaGameMode() const;
	void RefreshScore();
	void RefreshTickingText();

	struct FKillFeedItem
	{
		TWeakObjectPtr<UWidget> Widget;
		float ExpireTime = 0.f;
	};
	TArray<FKillFeedItem> KillFeedItems;

	TWeakObjectPtr<AArenaCharacter> BoundCharacter;
	TWeakObjectPtr<AArenaGameMode> BoundGameMode;
	float UITime = 0.f;
	float HitMarkerHideTime = 0.f;
	float AnnouncementHideTime = 0.f;
	float DamageFlashOpacity = 0.f;
};
