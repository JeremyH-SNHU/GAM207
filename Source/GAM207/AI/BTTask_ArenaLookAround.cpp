#include "AI/BTTask_ArenaLookAround.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/Pawn.h"

UBTTask_ArenaLookAround::UBTTask_ArenaLookAround()
{
	NodeName = TEXT("Look Around");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

uint16 UBTTask_ArenaLookAround::GetInstanceMemorySize() const
{
	return sizeof(FLookAroundMemory);
}

void UBTTask_ArenaLookAround::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FLookAroundMemory>(NodeMemory, InitType);
}

void UBTTask_ArenaLookAround::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FLookAroundMemory>(NodeMemory, CleanupType);
}

FString UBTTask_ArenaLookAround::GetStaticDescription() const
{
	return FString::Printf(TEXT("%.1fs, glance every %.1fs, up to %.0f deg"), Duration, GlanceInterval, MaxGlanceAngle);
}

EBTNodeResult::Type UBTTask_ArenaLookAround::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		return EBTNodeResult::Failed;
	}

	FLookAroundMemory* Memory = CastInstanceNodeMemory<FLookAroundMemory>(NodeMemory);
	Memory->TimeLeft = Duration;
	Memory->NextGlance = 0.f;
	Memory->BaseYaw = Pawn->GetActorRotation().Yaw;
	return EBTNodeResult::InProgress;
}

void UBTTask_ArenaLookAround::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = AIController ? AIController->GetPawn() : nullptr;
	if (!Pawn)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	FLookAroundMemory* Memory = CastInstanceNodeMemory<FLookAroundMemory>(NodeMemory);
	Memory->TimeLeft -= DeltaSeconds;
	Memory->NextGlance -= DeltaSeconds;

	if (Memory->NextGlance <= 0.f)
	{
		const float Yaw = Memory->BaseYaw + FMath::FRandRange(-MaxGlanceAngle, MaxGlanceAngle);
		const FVector LookPoint = Pawn->GetActorLocation() + FRotator(0.f, Yaw, 0.f).Vector() * 500.f;
		AIController->SetFocalPoint(LookPoint, EAIFocusPriority::Gameplay);
		Memory->NextGlance = GlanceInterval;
	}

	if (Memory->TimeLeft <= 0.f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_ArenaLookAround::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AAIController* AIController = OwnerComp.GetAIOwner())
	{
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
