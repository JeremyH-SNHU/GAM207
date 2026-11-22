#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_ArenaLookAround.generated.h"

/** Glances left and right for a few seconds, like someone checking a room. Used at the end of Search and Investigate moves. */
UCLASS(meta = (DisplayName = "Arena: Look Around"))
class GAM207_API UBTTask_ArenaLookAround : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_ArenaLookAround();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;
	virtual FString GetStaticDescription() const override;

	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "0.1"))
	float Duration = 2.f;

	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "0.1"))
	float GlanceInterval = 0.7f;

	/** Biggest turn away from the starting direction, in degrees. */
	UPROPERTY(EditAnywhere, Category = "Arena", meta = (ClampMin = "0", ClampMax = "180"))
	float MaxGlanceAngle = 110.f;

private:
	struct FLookAroundMemory
	{
		float TimeLeft = 0.f;
		float NextGlance = 0.f;
		float BaseYaw = 0.f;
	};
};
