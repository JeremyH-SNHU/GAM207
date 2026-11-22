#pragma once

#include "CoreMinimal.h"
#include "ArenaTypes.generated.h"

class AArenaCharacter;
class AArenaProjectile;

/** High-level mode the AI brain is in. The Behavior Tree branches on this (Blackboard key "AIState"). */
UENUM(BlueprintType)
enum class EArenaAIState : uint8
{
	Patrol       UMETA(ToolTip = "Nothing known: wander the arena."),
	Investigate  UMETA(ToolTip = "Heard a gunshot: go check it out."),
	Combat       UMETA(ToolTip = "Can see a target: fight it."),
	Search       UMETA(ToolTip = "Lost sight of (or was shot by) someone: hunt for them."),
	Flee         UMETA(ToolTip = "Too hurt or too scared: get away from the threat.")
};

/** How an enemy archetype approaches a fight. */
UENUM(BlueprintType)
enum class EArenaPersonality : uint8
{
	Aggressive  UMETA(ToolTip = "Closes distance, rushes toward gunfire, charges whoever shoots it."),
	Defensive   UMETA(ToolTip = "Holds its preferred range and approaches noises carefully."),
	Cowardly    UMETA(ToolTip = "Keeps far away, ignores noises when hurt, flees when hit or low on health.")
};

UENUM(BlueprintType)
enum class EArenaPickupType : uint8
{
	Heal         UMETA(ToolTip = "+Amount health."),
	MaxHealth    UMETA(ToolTip = "+/- Amount max health. Negative values are an injury."),
	DamageBoost  UMETA(ToolTip = "Damage x Amount for Duration seconds."),
	SpeedBoost   UMETA(ToolTip = "Walk speed x Amount for Duration seconds.")
};

UENUM(BlueprintType)
enum class EArenaMatchEndReason : uint8
{
	ScoreLimit,
	TimeLimit
};

/** Everything that makes one gun feel different from another. Shared by the player and every enemy archetype. */
USTRUCT(BlueprintType)
struct FArenaWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	FText WeaponName = NSLOCTEXT("Arena", "DefaultWeaponName", "Rifle");

	/** Damage dealt by each projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0"))
	float Damage = 10.f;

	/** Seconds between shots while the trigger is held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0.05"))
	float FireInterval = 0.25f;

	/** Projectiles per trigger pull. Above 1 makes it a shotgun. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "1", ClampMax = "20"))
	int32 ProjectilesPerShot = 1;

	/** Random cone half-angle in degrees. 0 = perfectly accurate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "0", ClampMax = "45"))
	float SpreadDegrees = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon", meta = (ClampMin = "100"))
	float ProjectileSpeed = 4000.f;

	/** Leave empty to use the character's Default Projectile Class. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<AArenaProjectile> ProjectileClass;

	/** How loud each shot is for AI hearing. 1 = normal. The listener's hearing range is multiplied by this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Noise", meta = (ClampMin = "0"))
	float NoiseLoudness = 1.f;

	/** Hard cap on how far the shot can be heard (0 = only the listener's hearing range applies). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Noise", meta = (ClampMin = "0"))
	float NoiseRange = 4000.f;
};

/** The pre-match options chosen on the main menu. Lives on the Game Instance so it survives level loads. */
USTRUCT(BlueprintType)
struct FArenaMatchSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (ClampMin = "0", ClampMax = "16"))
	int32 EnemyCount = 5;

	/** -1 = mixed (enemies get a spread of archetypes). Otherwise only that archetype (index into the archetype list) spawns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	int32 EnemyArchetypeIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Modifiers", meta = (ClampMin = "0.1", ClampMax = "10"))
	float EnemyHealthMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Modifiers", meta = (ClampMin = "0.1", ClampMax = "10"))
	float EnemyDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Modifiers", meta = (ClampMin = "0.1", ClampMax = "10"))
	float PlayerHealthMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Modifiers", meta = (ClampMin = "0.1", ClampMax = "10"))
	float PlayerDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Scoring", meta = (ClampMin = "0"))
	int32 PointsPerKill = 1;

	/** First to this score wins. 0 = no score limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Scoring", meta = (ClampMin = "0"))
	int32 ScoreLimit = 10;

	/** Match length in minutes. 0 = no time limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match|Scoring", meta = (ClampMin = "0"))
	float TimeLimitMinutes = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match", meta = (ClampMin = "0.5", ClampMax = "30"))
	float RespawnDelay = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match")
	bool bPickupsEnabled = true;
};

/** One row of the scoreboard. Kept by the game mode for every combatant. */
USTRUCT(BlueprintType)
struct FArenaCombatantStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	TObjectPtr<AArenaCharacter> Combatant = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bIsPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 Score = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ShotsFired = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ShotsHit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float DamageDealt = 0.f;

	float GetAccuracy() const { return ShotsFired > 0 ? static_cast<float>(ShotsHit) / static_cast<float>(ShotsFired) : 0.f; }
};

/** Blackboard key names the C++ code reads and writes. Your Blackboard asset must contain keys with exactly these names. */
namespace ArenaBlackboardKeys
{
	inline const FName SelfActor(TEXT("SelfActor"));
	inline const FName TargetActor(TEXT("TargetActor"));
	inline const FName AIState(TEXT("AIState"));
	inline const FName Personality(TEXT("Personality"));
	inline const FName LastKnownLocation(TEXT("LastKnownLocation"));
	inline const FName InvestigateLocation(TEXT("InvestigateLocation"));
	inline const FName MoveGoal(TEXT("MoveGoal"));
}
