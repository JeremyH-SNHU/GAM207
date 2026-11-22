#include "AI/BTTask_ArenaFindMoveGoal.h"
#include "AI/ArenaAIController.h"
#include "Characters/ArenaAICharacter.h"
#include "Gameplay/ArenaPickup.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"

namespace
{
	bool ProjectToNav(UNavigationSystemV1* Nav, const FVector& Point, FVector& Out, float Extent = 400.f)
	{
		FNavLocation Location;
		if (Nav->ProjectPointToNavigation(Point, Location, FVector(Extent, Extent, Extent)))
		{
			Out = Location.Location;
			return true;
		}
		return false;
	}

	bool RandomReachable(UNavigationSystemV1* Nav, const FVector& Origin, float Radius, FVector& Out)
	{
		FNavLocation Location;
		if (Nav->GetRandomReachablePointInRadius(Origin, Radius, Location))
		{
			Out = Location.Location;
			return true;
		}
		return false;
	}

	FVector RandomDirection2D()
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		return FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
	}

	/** Nearest available heal pickup within MaxDistance. If AvoidLocation is given, skip pickups that are closer to it than we are. */
	AArenaPickup* FindHealthPickup(UWorld* World, const FVector& From, float MaxDistance, const FVector* AvoidLocation)
	{
		AArenaPickup* Best = nullptr;
		float BestDistance = MaxDistance;
		for (TActorIterator<AArenaPickup> It(World); It; ++It)
		{
			AArenaPickup* Pickup = *It;
			if (!Pickup->IsAvailable() || Pickup->PickupType != EArenaPickupType::Heal)
			{
				continue;
			}
			const FVector PickupLocation = Pickup->GetActorLocation();
			if (AvoidLocation && FVector::Dist(PickupLocation, *AvoidLocation) < FVector::Dist(From, *AvoidLocation))
			{
				continue; // don't run toward the threat
			}
			const float Distance = FVector::Dist(From, PickupLocation);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = Pickup;
			}
		}
		return Best;
	}

	/** Walk toward Target in personality-sized steps; once close, sweep random points around it. */
	bool FindApproachGoal(UNavigationSystemV1* Nav, const FVector& MyLocation, const FVector& Target, EArenaPersonality Personality, float SweepRadius, FVector& Out)
	{
		const float Distance = FVector::Dist2D(MyLocation, Target);
		if (Distance > SweepRadius * 0.5f)
		{
			float StepFraction = 1.f;                                                      // aggressive: straight there
			if (Personality == EArenaPersonality::Defensive) { StepFraction = 0.6f; }      // creep up in stages
			if (Personality == EArenaPersonality::Cowardly)  { StepFraction = 0.4f; }

			const float Remaining = Distance * (1.f - StepFraction);
			const FVector Step = Remaining < 250.f ? Target : FMath::Lerp(MyLocation, Target, StepFraction);
			if (ProjectToNav(Nav, Step, Out))
			{
				return true;
			}
		}
		return RandomReachable(Nav, Target, SweepRadius, Out);
	}
}

UBTTask_ArenaFindMoveGoal::UBTTask_ArenaFindMoveGoal()
{
	NodeName = TEXT("Find Move Goal");
	GoalKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_ArenaFindMoveGoal, GoalKey));
	GoalKey.SelectedKeyName = ArenaBlackboardKeys::MoveGoal;
}

void UBTTask_ArenaFindMoveGoal::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	if (const UBlackboardData* BlackboardAsset = GetBlackboardAsset())
	{
		GoalKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

FString UBTTask_ArenaFindMoveGoal::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s -> %s"), *UEnum::GetDisplayValueAsText(Mode).ToString(), *GoalKey.SelectedKeyName.ToString());
}

EBTNodeResult::Type UBTTask_ArenaFindMoveGoal::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AArenaAIController* AIController = Cast<AArenaAIController>(OwnerComp.GetAIOwner());
	AArenaAICharacter* Me = AIController ? AIController->GetArenaPawn() : nullptr;
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	UNavigationSystemV1* Nav = Me ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(Me->GetWorld()) : nullptr;
	if (!Me || !BlackboardComp || !Nav)
	{
		return EBTNodeResult::Failed;
	}

	const FVector MyLocation = Me->GetActorLocation();
	const EArenaPersonality Personality = Me->GetPersonality();
	FVector Goal = MyLocation;
	bool bFound = false;

	switch (Mode)
	{
	case EArenaMoveGoalMode::Patrol:
	{
		if (Me->GetHealthFraction() < SeekHealthBelow)
		{
			if (AArenaPickup* Pickup = FindHealthPickup(Me->GetWorld(), MyLocation, PatrolRadius * 2.f, nullptr))
			{
				bFound = ProjectToNav(Nav, Pickup->GetActorLocation(), Goal);
			}
		}
		if (!bFound)
		{
			bFound = RandomReachable(Nav, MyLocation, PatrolRadius, Goal);
		}
		break;
	}

	case EArenaMoveGoalMode::Combat:
	{
		const AArenaCharacter* Target = AIController->GetCurrentTarget();
		if (!Target)
		{
			break;
		}
		const FVector TargetLocation = Target->GetActorLocation();
		const float Distance = FVector::Dist2D(MyLocation, TargetLocation);
		const float Preferred = Me->GetPreferredRange();

		FVector AwayFromTarget = (MyLocation - TargetLocation).GetSafeNormal2D();
		if (AwayFromTarget.IsNearlyZero())
		{
			AwayFromTarget = RandomDirection2D();
		}

		float DesiredDistance = Preferred;
		switch (Personality)
		{
		case EArenaPersonality::Aggressive:
			DesiredDistance = Preferred * 0.6f;                                    // push in close
			break;
		case EArenaPersonality::Defensive:
			DesiredDistance = Distance < Preferred * 0.7f ? Preferred * 1.1f : Preferred; // back off if rushed
			break;
		case EArenaPersonality::Cowardly:
			DesiredDistance = FMath::Max(Preferred * 1.2f, Distance);              // never closer than now
			break;
		}

		// Side-step around the target (circle-strafe) so enemies aren't standing targets.
		const float StrafeDegrees = FMath::RadiansToDegrees(StrafeDistance / FMath::Max(DesiredDistance, 200.f)) * FMath::FRandRange(-1.f, 1.f);
		const FVector Direction = AwayFromTarget.RotateAngleAxis(StrafeDegrees, FVector::UpVector);
		FVector Candidate = TargetLocation + Direction * DesiredDistance;
		Candidate.Z = MyLocation.Z;

		bFound = ProjectToNav(Nav, Candidate, Goal) || RandomReachable(Nav, Candidate, 300.f, Goal) || RandomReachable(Nav, MyLocation, 300.f, Goal);
		break;
	}

	case EArenaMoveGoalMode::Search:
		bFound = FindApproachGoal(Nav, MyLocation, AIController->GetSearchLocation(), Personality, SearchRadius, Goal);
		break;

	case EArenaMoveGoalMode::Investigate:
		bFound = FindApproachGoal(Nav, MyLocation, AIController->GetInvestigateLocation(), Personality, SearchRadius, Goal);
		break;

	case EArenaMoveGoalMode::Flee:
	{
		const FVector Threat = AIController->GetThreatLocation();

		// A health pickup on the far side of us from the threat is the best place to run.
		if (AArenaPickup* Pickup = FindHealthPickup(Me->GetWorld(), MyLocation, FleeDistance * 1.5f, &Threat))
		{
			bFound = ProjectToNav(Nav, Pickup->GetActorLocation(), Goal);
		}
		if (bFound)
		{
			break;
		}

		// Otherwise try a fan of directions pointing away from the threat and keep the farthest one.
		FVector Away = (MyLocation - Threat).GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = RandomDirection2D();
		}
		float BestDistance = -1.f;
		for (int32 Index = 0; Index < 7; ++Index)
		{
			const float Degrees = -90.f + Index * 30.f;
			FVector Candidate;
			if (ProjectToNav(Nav, MyLocation + Away.RotateAngleAxis(Degrees, FVector::UpVector) * FleeDistance, Candidate))
			{
				const float Distance = FVector::Dist2D(Candidate, Threat);
				if (Distance > BestDistance)
				{
					BestDistance = Distance;
					Goal = Candidate;
					bFound = true;
				}
			}
		}
		if (!bFound)
		{
			bFound = RandomReachable(Nav, MyLocation, FleeDistance, Goal);
		}
		break;
	}
	}

	if (!bFound)
	{
		return EBTNodeResult::Failed;
	}
	BlackboardComp->SetValueAsVector(GoalKey.SelectedKeyName, Goal);
	return EBTNodeResult::Succeeded;
}
