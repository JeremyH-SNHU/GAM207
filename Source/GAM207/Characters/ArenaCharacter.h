#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArenaTypes.h"
#include "ArenaCharacter.generated.h"

class AArenaGameMode;
class AArenaProjectile;
class UAnimSequenceBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FArenaHealthChangedSignature, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FArenaDamagedSignature, AArenaCharacter*, Attacker, float, Damage, FVector, HitDirection);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FArenaHitLandedSignature, AArenaCharacter*, Victim, float, Damage, bool, bKilled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FArenaDiedSignature, AArenaCharacter*, Killer);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FArenaSimpleSignature);

/**
 * Shared base for the player and every enemy: health, damage, the weapon, buffs, hit reactions, death and respawn.
 * Scoring and respawn timing live in AArenaGameMode; this class just reports what happened to it.
 */
UCLASS(Abstract)
class GAM207_API AArenaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AArenaCharacter();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// ---------------------------------------------------------------- Health

	UFUNCTION(BlueprintPure, Category = "Arena|Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Arena|Health")
	float GetMaxHealth() const { return CurrentMaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Arena|Health")
	float GetHealthFraction() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Health")
	bool IsAlive() const { return !bIsDead; }

	/** Restores health up to max. Returns true if any health was restored. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	bool Heal(float Amount);

	/** Raises or lowers max health until the next respawn. Never kills: health is kept at 1 or above. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Health")
	void ModifyMaxHealth(float Delta, bool bAlsoChangeHealth = true);

	// ---------------------------------------------------------------- Buffs

	UFUNCTION(BlueprintCallable, Category = "Arena|Buffs")
	void ApplyDamageBoost(float Multiplier, float Duration);

	UFUNCTION(BlueprintCallable, Category = "Arena|Buffs")
	void ApplySpeedBoost(float Multiplier, float Duration);

	UFUNCTION(BlueprintPure, Category = "Arena|Buffs")
	float GetDamageBoostMultiplier() const { return DamageBoostMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Arena|Buffs")
	float GetSpeedBoostMultiplier() const { return SpeedBoostMultiplier; }

	UFUNCTION(BlueprintPure, Category = "Arena|Buffs")
	float GetDamageBoostTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Buffs")
	float GetSpeedBoostTimeRemaining() const;

	// ---------------------------------------------------------------- Weapon

	/** Fires immediately (if the gun is ready) and keeps firing every Weapon.FireInterval until StopFiring. */
	UFUNCTION(BlueprintCallable, Category = "Arena|Weapon")
	void StartFiring();

	UFUNCTION(BlueprintCallable, Category = "Arena|Weapon")
	void StopFiring();

	UFUNCTION(BlueprintPure, Category = "Arena|Weapon")
	bool IsFiring() const;

	// ---------------------------------------------------------------- Match (called by the game mode)

	UFUNCTION(BlueprintPure, Category = "Arena")
	FText GetCombatantName() const { return CombatantName; }

	UFUNCTION(BlueprintPure, Category = "Arena")
	float GetRespawnTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Arena")
	bool IsFrozen() const { return bFrozen; }

	void SetCombatantName(const FText& InName) { CombatantName = InName; }
	void SetMatchModifiers(float HealthMultiplier, float DamageMultiplier);
	void SetRespawnTime(float WorldTime) { RespawnAtTime = WorldTime; }
	void RespawnAt(const FTransform& SpawnTransform);
	void FreezeForMatchEnd();
	void NotifyHitLanded(AArenaCharacter* Victim, float Damage, bool bKilled);

	// ---------------------------------------------------------------- Tuning

	/** Where projectiles spawn. Move it in the Blueprint viewport to line up with the gun. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|Weapon")
	TObjectPtr<USceneComponent> MuzzlePoint;

	/** Max health before match modifiers (enemies take this from their archetype instead). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|Health", meta = (ClampMin = "1"))
	float BaseMaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Weapon")
	FArenaWeaponStats Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena|Weapon")
	TSubclassOf<AArenaProjectile> DefaultProjectileClass;

	/** One is picked at random when hit. Needs a "DefaultSlot" Slot node in the Anim Blueprint's AnimGraph. */
	UPROPERTY(EditAnywhere, Category = "Arena|Animation")
	TArray<TObjectPtr<UAnimSequenceBase>> HitReactAnimations;

	UPROPERTY(EditAnywhere, Category = "Arena|Animation")
	FName HitReactSlotName = TEXT("DefaultSlot");

	UPROPERTY(EditAnywhere, Category = "Arena|Animation", meta = (ClampMin = "0"))
	float HitReactCooldown = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Arena|Animation")
	bool bRagdollOnDeath = true;

	/** Velocity (cm/s) added to the ragdoll in the direction of the killing shot. */
	UPROPERTY(EditAnywhere, Category = "Arena|Animation", meta = (ClampMin = "0"))
	float DeathImpulse = 350.f;

	// ---------------------------------------------------------------- Events (bind to these from UI or Blueprints)

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaDamagedSignature OnDamaged;

	/** Fires on the shooter whenever one of its projectiles hurts someone. Drives the hit marker. */
	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaHitLandedSignature OnHitLanded;

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaDiedSignature OnDied;

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaSimpleSignature OnRespawned;

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaSimpleSignature OnBuffsChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Runs in BeginPlay before the character registers with the game mode. Subclasses do their setup here. */
	virtual void InitializeCombatant() {}

	/** Runs right after the game mode has registered this character (so its name is known). */
	virtual void PostCombatantRegistered() {}

	/** The point the weapon should aim at this shot. */
	virtual FVector GetAimLocation(const FVector& MuzzleLocation) const;

	virtual void OnDeath(AArenaCharacter* Killer) {}
	virtual void OnRespawn() {}

	void FireShot();
	void Die(AArenaCharacter* Killer, const FVector& HitDirection);
	void PlayHitReact();
	void BroadcastHealth();
	void UpdateWalkSpeed();
	void ClearBuffs();
	void EndDamageBoost();
	void EndSpeedBoost();
	AArenaGameMode* GetArenaGameMode() const;
	static AArenaCharacter* ResolveAttacker(AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena", meta = (DisplayName = "On Weapon Fired"))
	void BP_OnWeaponFired(FVector MuzzleLocation, FVector AimDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena", meta = (DisplayName = "On Took Damage"))
	void BP_OnTookDamage(AArenaCharacter* Attacker, float Damage, FVector HitDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena", meta = (DisplayName = "On Died"))
	void BP_OnDied(AArenaCharacter* Killer);

	UFUNCTION(BlueprintImplementableEvent, Category = "Arena", meta = (DisplayName = "On Respawned"))
	void BP_OnRespawned();

	float BaseWalkSpeed = 600.f;
	FTransform MeshRelativeTransform;

private:
	float Health = 100.f;
	float CurrentMaxHealth = 100.f;
	float MatchHealthMultiplier = 1.f;
	float MatchDamageMultiplier = 1.f;
	float DamageBoostMultiplier = 1.f;
	float SpeedBoostMultiplier = 1.f;
	bool bIsDead = false;
	bool bFrozen = false;
	float LastFireTime = -1000.f;
	float LastHitReactTime = -1000.f;
	float RespawnAtTime = 0.f;
	FText CombatantName;
	FName MeshCollisionProfile;
	FTimerHandle FireTimer;
	FTimerHandle DamageBoostTimer;
	FTimerHandle SpeedBoostTimer;
};
