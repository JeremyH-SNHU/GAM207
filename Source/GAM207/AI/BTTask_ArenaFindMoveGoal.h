#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BTTask_ArenaFindMoveGoal.generated.h"

UENUM()
enum class EArenaMoveGoalMode : uint8
{
	Patrol       UMETA(ToolTip = "Random reachable point. Heads for a health pickup when hurt."),
	Combat       UMETA(ToolTip = "A position around the target based on personality and preferred range."),
	Search       UMETA(ToolTip = "Approach the predicted position of the lost target, then sweep around it."),
	Investigate  UMETA(ToolTip = "Approach the last heard gunshot, then sweep around it."),
	Flee         UMETA(ToolTip = "Somewhere far from the threat, preferring a health pickup.")
};

/**
 * Picks where the enemy should move next and writes it to a vector Blackboard key (MoveGoal by default).
 * Follow it with a Move To on the same key. Personality changes the answer: aggressive enemies close in
 * and walk straight to noises, defensive ones hold range and creep closer, cowardly ones keep their distance.
 */
UCLASS(meta = (DisplayName = "Arena: Find Move Goal"))
class GAM207_API UBTTask_ArenaFindMoveGoal : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ArenaFindMoveGoal();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;

	UPROPERTY(EditAnywhere, Category = "Arena")
	EArenaMoveGoalMode Mode = EArenaMoveGoalMode::Patrol;

	/** Vector key that receives the chosen location. */
	UPROPERTY(EditAnywhere, Category = "Arena")
	FBlackboardKeySelector GoalKey;

	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "100"))
	float PatrolRadius = 2500.f;

	/** How far around the search/investigate point to sweep once there. */
	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "100"))
	float SearchRadius = 700.f;

	/** How far to side-step around the target each combat move. */
	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "0"))
	float StrafeDistance = 350.f;

	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "100"))
	float FleeDistance = 1500.f;

	/** While patrolling below this health fraction, walk to the nearest health pickup instead. */
	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "0", ClampMax = "1"))
	float SeekHealthBelow = 0.6f;
};
