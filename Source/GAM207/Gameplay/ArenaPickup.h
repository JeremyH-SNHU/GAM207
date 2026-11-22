#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArenaTypes.h"
#include "ArenaPickup.generated.h"

class AArenaCharacter;

/**
 * Base class for every pickup. Reparent Pickup_Master to this, then set Pickup Type / Amount / Duration
 * in each child's Class Defaults. Works for the player and the AI alike (no separate "Player" and "AI" events).
 */
UCLASS()
class GAM207_API AArenaPickup : public AActor
{
	GENERATED_BODY()

public:
	AArenaPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Pickup")
	EArenaPickupType PickupType = EArenaPickupType::Heal;

	/** Heal: health restored. MaxHealth: change to max health (negative = injury). Boosts: multiplier (2 = double). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Pickup")
	float Amount = 25.f;

	/** Boost length in seconds. Ignored by Heal and MaxHealth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Pickup", meta = (ClampMin = "0"))
	float Duration = 8.f;

	/** Seconds until the pickup comes back. 0 = one use only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Pickup", meta = (ClampMin = "0"))
	float RespawnTime = 15.f;

	/** Heal pickups are left alone by characters already at full health. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Pickup")
	bool bOnlyWhenNeeded = true;

	/** Applies the effect. Returns false if it didn't apply (e.g. full health), in which case the pickup stays. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Pickup")
	bool TryApplyTo(AArenaCharacter* Character);

	UFUNCTION(BlueprintPure, Category = "Arena|Pickup")
	bool IsAvailable() const { return bAvailable && !bDisabledForMatch; }

	/** Used by the game mode when pickups are switched off in the match settings. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Pickup")
	void SetPickupEnabled(bool bEnabled);

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Pickup", meta = (DisplayName = "On Picked Up"))
	void BP_OnPickedUp(AArenaCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena|Pickup", meta = (DisplayName = "On Pickup Respawned"))
	void BP_OnRespawned();

private:
	void Consume();
	void Respawn();
	void SetShown(bool bShown);

	bool bAvailable = true;
	bool bDisabledForMatch = false;
	FTimerHandle RespawnTimer;
};
