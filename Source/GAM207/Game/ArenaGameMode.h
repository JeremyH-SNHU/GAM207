#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArenaTypes.h"
#include "ArenaGameMode.generated.h"

class AArenaAICharacter;
class AArenaAIController;
class AArenaCharacter;
class UArenaEnemyArchetype;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FArenaKillSignature, AArenaCharacter*, Killer, AArenaCharacter*, Victim, int32, PointsAwarded);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FArenaScoresChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FArenaMatchEndedSignature, AArenaCharacter*, Winner, EArenaMatchEndReason, Reason);

/**
 * Free-for-all referee. Reparent ArenaMode to this.
 * Reads the match settings from the Game Instance, spawns the enemies, keeps score, respawns the dead,
 * and ends the match on the score or time limit.
 */
UCLASS()
class GAM207_API AArenaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArenaGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void StartPlay() override;

	// ---------------------------------------------------------------- Queries (for UI)

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	FArenaMatchSettings GetMatchSettings() const { return Settings; }

	/** Every combatant, best first (score, then kills, then fewest deaths). */
	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	TArray<FArenaCombatantStats> GetSortedStats() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	bool GetStatsFor(const AArenaCharacter* Combatant, FArenaCombatantStats& OutStats) const;

	/** 1 = first place. 0 if the combatant isn't registered. */
	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	int32 GetRankOf(const AArenaCharacter* Combatant) const;

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	int32 GetCombatantCount() const { return Combatants.Num(); }

	/** Seconds left, or -1 when there is no time limit. */
	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	float GetTimeRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	float GetMatchElapsedTime() const;

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	bool IsMatchOver() const { return bMatchOver; }

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	AArenaCharacter* GetWinner() const { return Winner.Get(); }

	UFUNCTION(BlueprintPure, Category = "Arena|Match")
	EArenaMatchEndReason GetEndReason() const { return EndReason; }

	UFUNCTION(BlueprintCallable, Category = "Arena|Match")
	void EndMatch(EArenaMatchEndReason Reason);

	// ---------------------------------------------------------------- Events

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaKillSignature OnKill;

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaScoresChangedSignature OnScoresChanged;

	UPROPERTY(BlueprintAssignable, Category = "Arena|Events")
	FArenaMatchEndedSignature OnMatchEnded;

	// ---------------------------------------------------------------- Called by characters

	void RegisterCombatant(AArenaCharacter* Combatant);
	void HandleCombatantDeath(AArenaCharacter* Victim, AArenaCharacter* Killer);
	void RecordShotsFired(AArenaCharacter* Shooter, int32 Count);
	void RecordHit(AArenaCharacter* Shooter, float Damage);

	// ---------------------------------------------------------------- Setup

	/** Enemy Blueprint to spawn. Defaults to /Game/AI/AICharacter_BP (once it's reparented to ArenaAICharacter). */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	TSoftClassPtr<AArenaAICharacter> EnemyClass;

	/** Controller given to every spawned enemy, whatever its Blueprint says. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	TSubclassOf<AArenaAIController> EnemyControllerClass;

	/** Actors of this class are used as spawn/respawn points. Falls back to Player Starts. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	TSoftClassPtr<AActor> SpawnPointClass;

	/** Destroy enemies placed by hand in the level so the match settings decide how many there are. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Spawning")
	bool bReplacePlacedEnemies = true;

	/** Pickups of this class are removed when pickups are disabled, even if they aren't ArenaPickups yet. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Pickups")
	TSoftClassPtr<AActor> LegacyPickupClass;

	/** Time dilation during the short pause before the end screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Match", meta = (ClampMin = "0.05", ClampMax = "1"))
	float EndMatchSlowMotion = 0.25f;

	/** Real-time seconds between the match ending and the end screen appearing. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|Match", meta = (ClampMin = "0"))
	float EndScreenDelay = 2.f;

private:
	FArenaCombatantStats* FindStats(const AArenaCharacter* Combatant);
	const FArenaCombatantStats* FindStats(const AArenaCharacter* Combatant) const;
	UClass* ResolveEnemyClass() const;
	void RemovePlacedEnemies();
	void CacheSpawnPoints();
	void SpawnEnemies();
	void ApplyPickupSetting();
	UArenaEnemyArchetype* PickArchetype(int32 SpawnIndex) const;
	FTransform ChooseSpawnTransform(const AArenaCharacter* ForCombatant) const;
	void RespawnCombatant(TWeakObjectPtr<AArenaCharacter> Combatant);
	void HandleTimeLimitReached();
	void ShowEndScreens();

	UPROPERTY(Transient)
	TArray<FArenaCombatantStats> Combatants;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UArenaEnemyArchetype>> Archetypes;

	TArray<TWeakObjectPtr<AActor>> SpawnPoints;
	TMap<FString, int32> NameCounts;
	FArenaMatchSettings Settings;
	TWeakObjectPtr<AArenaCharacter> Winner;
	EArenaMatchEndReason EndReason = EArenaMatchEndReason::ScoreLimit;
	float MatchStartTime = 0.f;
	float MatchEndTime = -1.f;
	bool bMatchOver = false;
	int32 ArchetypeOffset = 0;
	FTimerHandle TimeLimitTimer;
	FTimerHandle EndScreenTimer;
};
