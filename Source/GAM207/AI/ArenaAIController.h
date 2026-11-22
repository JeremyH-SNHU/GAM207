#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ArenaTypes.h"
#include "ArenaAIController.generated.h"

class AArenaAICharacter;
class AArenaCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBehaviorTree;

/** Focus priority for the combat target. Sits above Gameplay so BT tasks that clear Gameplay focus (Look Around) never drop the target. */
namespace ArenaFocusPriority
{
	inline const EAIFocusPriority::Type Target = static_cast<EAIFocusPriority::Type>(EAIFocusPriority::Gameplay + 1);
}

/**
 * The enemy brain.
 *
 *  Senses (AIPerception): Sight, Hearing (gunshots) and Damage (being shot).
 *  Memory: every enemy it has seen or been shot by is remembered with a last-known location and velocity.
 *          Losing sight starts a Search at the predicted position instead of forgetting the target; a gunshot
 *          from a remembered enemy refreshes that position. Memories fade after the archetype's Memory Duration.
 *  Decisions: every ThinkInterval it picks a state (Patrol / Investigate / Combat / Search / Flee) and a target,
 *          handles aiming and firing, and writes the result to the Blackboard. The Behavior Tree only moves the pawn.
 */
UCLASS()
class GAM207_API AArenaAIController : public AAIController
{
	GENERATED_BODY()

public:
	AArenaAIController();

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	AArenaCharacter* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	EArenaAIState GetAIState() const { return State; }

	/** Where the danger is (used when fleeing). */
	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	FVector GetThreatLocation() const { return ThreatLocation; }

	/** Predicted position of the enemy being searched for. */
	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	FVector GetSearchLocation() const { return SearchLocation; }

	/** Where the last interesting noise came from. */
	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	FVector GetInvestigateLocation() const { return InvestigateLocation; }

	UFUNCTION(BlueprintPure, Category = "Arena|AI")
	AArenaAICharacter* GetArenaPawn() const;

	void OnPawnDied();
	void OnPawnRespawned();
	void OnMatchEnded();

	/** Behavior Tree to run. Defaults to /Game/AI/BT_Arena. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI")
	TSoftObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arena|AI")
	TObjectPtr<UAIPerceptionComponent> ArenaPerception;

	UPROPERTY(VisibleAnywhere, Category = "Arena|AI")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "Arena|AI")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY(VisibleAnywhere, Category = "Arena|AI")
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	/** Seconds between decisions. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning", meta = (ClampMin = "0.02"))
	float ThinkInterval = 0.1f;

	/** How long a heard gunshot stays interesting. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning", meta = (ClampMin = "0"))
	float InvestigateDuration = 8.f;

	/** Cowardly enemies run for this long after being shot. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning", meta = (ClampMin = "0"))
	float FleeOnHitDuration = 3.f;

	/** Cowardly enemies below this health fraction ignore gunshots instead of investigating. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning", meta = (ClampMin = "0", ClampMax = "1"))
	float CowardInvestigateHealth = 0.75f;

	/** When sight is lost, the search starts this many seconds ahead along the target's last velocity. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning", meta = (ClampMin = "0"))
	float PredictionTime = 1.f;

	/** Target-choice bonus (in cm of distance) for whoever shot us in the last few seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning")
	float AttackerPriorityBonus = 1500.f;

	/** Target-choice bonus for the current target, so enemies don't flick between targets. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning")
	float TargetStickiness = 400.f;

	/** Only fire when the target is within this many degrees of where the pawn is facing. */
	UPROPERTY(EditDefaultsOnly, Category = "Arena|AI|Tuning", meta = (ClampMin = "1", ClampMax = "180"))
	float FireConeDegrees = 25.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	struct FTargetMemory
	{
		TWeakObjectPtr<AArenaCharacter> Actor;
		FVector LastKnownLocation = FVector::ZeroVector;
		FVector LastKnownVelocity = FVector::ZeroVector;
		float LastSeenTime = 0.f;
		float LastDamagedMeTime = -1000.f;
		bool bVisible = false;
	};

	void Think();
	void HandleHeardNoise(AArenaCharacter* Source, const FVector& NoiseLocation, float Now);
	void HandleDamagedBy(AArenaCharacter* Attacker, const FVector& AttackerLocation, float Now);
	void SetCurrentTarget(AArenaCharacter* NewTarget, float Now);
	void UpdateFiring(float Now);
	void UpdateBlackboard();
	void ValidateBlackboard() const;
	FVector PredictLocation(const FTargetMemory& Memory) const;
	FTargetMemory* FindMemory(const AArenaCharacter* Actor);
	FTargetMemory& FindOrAddMemory(AArenaCharacter* Actor, float Now);
	void ResetBrain();

	TArray<FTargetMemory> Memories;
	TWeakObjectPtr<AArenaCharacter> CurrentTarget;
	EArenaAIState State = EArenaAIState::Patrol;
	FVector ThreatLocation = FVector::ZeroVector;
	FVector SearchLocation = FVector::ZeroVector;
	FVector InvestigateLocation = FVector::ZeroVector;
	float TargetAcquiredTime = 0.f;
	float InvestigateUntil = -1000.f;
	float FleeUntil = -1000.f;
	float LastDamagedTime = -1000.f;
	bool bMatchOver = false;
	FTimerHandle ThinkTimer;
};
