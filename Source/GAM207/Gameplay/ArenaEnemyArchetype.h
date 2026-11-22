#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArenaTypes.h"
#include "ArenaEnemyArchetype.generated.h"

/**
 * One kind of enemy: its look, stats, gun and personality.
 * Create these in the Content Browser: right-click > Miscellaneous > Data Asset > ArenaEnemyArchetype.
 * If none exist, the game uses five built-in archetypes (see CreateBuiltInArchetypes).
 */
UCLASS(BlueprintType)
class GAM207_API UArenaEnemyArchetype : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName = NSLOCTEXT("Arena", "DefaultArchetypeName", "Soldier");

	/** Tints the mannequin's "BodyColor" material parameter so archetypes are easy to tell apart. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor BodyColor = FLinearColor(0.15f, 0.3f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float BodyScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "50"))
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FArenaWeaponStats Weapon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	EArenaPersonality Personality = EArenaPersonality::Defensive;

	/** Distance this enemy likes to fight from. Aggressive enemies push inside it; defensive ones hold it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "100"))
	float PreferredRange = 1000.f;

	/** Flee when health falls to this fraction while threatened (0 = never flee from low health). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0", ClampMax = "1"))
	float FleeHealthFraction = 0.f;

	/** Seconds this enemy keeps hunting someone after losing track of them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0"))
	float MemoryDuration = 10.f;

	/** Delay between first spotting a target and opening fire. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior", meta = (ClampMin = "0"))
	float ReactionTime = 0.35f;

	/** Fills OutArchetypes with the five default enemy types. Used when the project has no archetype Data Assets. */
	static void CreateBuiltInArchetypes(UObject* Outer, TArray<TObjectPtr<UArenaEnemyArchetype>>& OutArchetypes);
};
