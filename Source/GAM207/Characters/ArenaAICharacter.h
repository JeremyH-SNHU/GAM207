#pragma once

#include "CoreMinimal.h"
#include "Characters/ArenaCharacter.h"
#include "ArenaAICharacter.generated.h"

class UArenaEnemyArchetype;
class UWidgetComponent;

/**
 * An enemy. Reparent AICharacter_BP to this. Stats, gun, colour and personality come from its Archetype,
 * which the game mode assigns when it spawns the enemy.
 */
UCLASS()
class GAM207_API AArenaAICharacter : public AArenaCharacter
{
	GENERATED_BODY()

public:
	AArenaAICharacter();

	/** Must be called before BeginPlay (the game mode does this between SpawnActorDeferred and FinishSpawning). */
	void SetArchetype(UArenaEnemyArchetype* InArchetype) { Archetype = InArchetype; }

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	UArenaEnemyArchetype* GetArchetype() const { return Archetype; }

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	EArenaPersonality GetPersonality() const;

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	float GetPreferredRange() const;

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	float GetFleeHealthFraction() const;

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	float GetMemoryDuration() const;

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	float GetReactionTime() const;

	/** Can be set on an enemy placed by hand in the level. Spawned enemies get theirs from the game mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|AI")
	TObjectPtr<UArenaEnemyArchetype> Archetype;

	/** Used when no archetype is assigned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arena|AI")
	EArenaPersonality FallbackPersonality = EArenaPersonality::Defensive;

	/** Vector material parameter that receives the archetype's body colour. */
	UPROPERTY(EditAnywhere, Category = "Arena|AI")
	FName BodyColorParameter = TEXT("BodyColor");

	/** 0 = aim where the target is, 1 = aim fully where it will be when the bullet arrives. */
	UPROPERTY(EditAnywhere, Category = "Arena|AI", meta = (ClampMin = "0", ClampMax = "1"))
	float AimLeadFactor = 0.75f;

protected:
	virtual void InitializeCombatant() override;
	virtual void PostCombatantRegistered() override;
	virtual FVector GetAimLocation(const FVector& MuzzleLocation) const override;
	virtual void OnDeath(AArenaCharacter* Killer) override;
	virtual void OnRespawn() override;

private:
	void ApplyBodyColor(const FLinearColor& Color);

	TWeakObjectPtr<UWidgetComponent> OverheadWidgetComponent;
};
