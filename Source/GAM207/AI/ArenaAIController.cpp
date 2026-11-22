#include "AI/ArenaAIController.h"
#include "GAM207.h"
#include "Characters/ArenaAICharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "TimerManager.h"

AArenaAIController::AArenaAIController()
{
	BehaviorTreeAsset = TSoftObjectPtr<UBehaviorTree>(FSoftObjectPath(TEXT("/Game/AI/BT_Arena.BT_Arena")));

	ArenaPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("ArenaPerception"));

	// There are no teams in a free-for-all, so everyone is "Neutral": every sense has to detect neutrals.
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 3500.f;
	SightConfig->LoseSightRadius = 4000.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(0.f); // never expires; our own memory decides when to forget

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 4000.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(3.f);

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(5.f);

	ArenaPerception->ConfigureSense(*SightConfig);
	ArenaPerception->ConfigureSense(*HearingConfig);
	ArenaPerception->ConfigureSense(*DamageConfig);
	ArenaPerception->SetDominantSense(SightConfig->GetSenseImplementation());
	SetPerceptionComponent(*ArenaPerception);
}

AArenaAICharacter* AArenaAIController::GetArenaPawn() const
{
	return Cast<AArenaAICharacter>(GetPawn());
}

void AArenaAIController::BeginPlay()
{
	Super::BeginPlay();
	ArenaPerception->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &AArenaAIController::HandleTargetPerceptionUpdated);
}

void AArenaAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ResetBrain();
	bMatchOver = false;

	if (UBehaviorTree* Tree = BehaviorTreeAsset.LoadSynchronous())
	{
		RunBehaviorTree(Tree);
		ValidateBlackboard();
	}
	else
	{
		static bool bWarned = false;
		if (!bWarned)
		{
			bWarned = true;
			const FString Message = FString::Printf(TEXT("ArenaAIController: Behavior Tree '%s' not found. Enemies will stand still. Create it (see Docs/Editor_Setup_Guide.md) or set Behavior Tree Asset."), *BehaviorTreeAsset.ToString());
			UE_LOG(LogArena, Error, TEXT("%s"), *Message);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, Message);
			}
		}
	}

	// Stagger the first think so a dozen enemies don't all decide on the same frame.
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &AArenaAIController::Think, ThinkInterval, true, FMath::FRandRange(0.f, ThinkInterval));
}

void AArenaAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	ResetBrain();
	Super::OnUnPossess();
}

void AArenaAIController::ValidateBlackboard() const
{
	static bool bWarned = false;
	const UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (bWarned || !BlackboardComp || !BlackboardComp->GetBlackboardAsset())
	{
		return;
	}

	TArray<FString> Missing;
	for (const FName& Key : { ArenaBlackboardKeys::TargetActor, ArenaBlackboardKeys::AIState, ArenaBlackboardKeys::Personality,
		ArenaBlackboardKeys::LastKnownLocation, ArenaBlackboardKeys::InvestigateLocation, ArenaBlackboardKeys::MoveGoal })
	{
		if (BlackboardComp->GetKeyID(Key) == FBlackboard::InvalidKey)
		{
			Missing.Add(Key.ToString());
		}
	}

	if (Missing.Num() > 0)
	{
		bWarned = true;
		const FString Message = FString::Printf(TEXT("ArenaAIController: Blackboard '%s' is missing keys: %s"),
			*BlackboardComp->GetBlackboardAsset()->GetName(), *FString::Join(Missing, TEXT(", ")));
		UE_LOG(LogArena, Error, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, Message);
		}
	}
}

// ==================================================================== Perception

void AArenaAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	AArenaAICharacter* Me = GetArenaPawn();
	AArenaCharacter* Other = Cast<AArenaCharacter>(Actor);
	if (!Me || !Me->IsAlive() || bMatchOver || !Other || Other == Me || !Other->IsAlive())
	{
		return;
	}

	bool bWasShot = false;
	if (Stimulus.WasSuccessfullySensed() && !Stimulus.IsExpired())
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
		{
			HandleHeardNoise(Other, Stimulus.StimulusLocation, Now);
		}
		else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
		{
			HandleDamagedBy(Other, Stimulus.StimulusLocation, Now);
			bWasShot = true;
		}
	}

	// Sight changes (gained or lost) are picked up by Think, which reads the perception component directly.
	// Thinking now instead of waiting for the timer makes reactions feel instant.
	Think();

	// Shot by someone we can't see: spin toward where it came from straight away. Done after Think,
	// because a state change inside Think clears the Gameplay focus.
	if (bWasShot && !CurrentTarget.IsValid() && State != EArenaAIState::Flee)
	{
		SetFocalPoint(Stimulus.StimulusLocation, EAIFocusPriority::Gameplay);
	}
}

void AArenaAIController::HandleHeardNoise(AArenaCharacter* Source, const FVector& NoiseLocation, float Now)
{
	// A gunshot from someone we're already hunting tells us where they went. This is what keeps
	// enemies from forgetting you: shooting while out of sight updates their idea of where you are.
	if (FTargetMemory* Memory = FindMemory(Source))
	{
		if (!Memory->bVisible)
		{
			Memory->LastKnownLocation = NoiseLocation;
			Memory->LastKnownVelocity = FVector::ZeroVector;
			Memory->LastSeenTime = Now;
		}
		return;
	}

	const AArenaAICharacter* Me = GetArenaPawn();
	if (Me->GetPersonality() == EArenaPersonality::Cowardly && Me->GetHealthFraction() < CowardInvestigateHealth)
	{
		return;
	}

	InvestigateLocation = NoiseLocation;
	InvestigateUntil = Now + InvestigateDuration;
}

void AArenaAIController::HandleDamagedBy(AArenaCharacter* Attacker, const FVector& AttackerLocation, float Now)
{
	LastDamagedTime = Now;

	// Being shot always tells us roughly where the shooter is, even if we never saw them.
	FTargetMemory& Memory = FindOrAddMemory(Attacker, Now);
	Memory.LastDamagedMeTime = Now;
	if (!Memory.bVisible)
	{
		Memory.LastKnownLocation = AttackerLocation;
		Memory.LastKnownVelocity = FVector::ZeroVector;
		Memory.LastSeenTime = Now;
	}
	ThreatLocation = AttackerLocation;

	const AArenaAICharacter* Me = GetArenaPawn();
	if (Me->GetPersonality() == EArenaPersonality::Cowardly)
	{
		FleeUntil = Now + FleeOnHitDuration;
	}
}

// ==================================================================== Decisions

void AArenaAIController::Think()
{
	AArenaAICharacter* Me = GetArenaPawn();
	if (!Me || !Me->IsAlive() || bMatchOver)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	const FVector MyLocation = Me->GetActorLocation();

	// 1) Who can we see right now?
	TArray<AActor*> Seen;
	ArenaPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Seen);
	for (AActor* Actor : Seen)
	{
		AArenaCharacter* Other = Cast<AArenaCharacter>(Actor);
		if (Other && Other != Me && Other->IsAlive())
		{
			FindOrAddMemory(Other, Now);
		}
	}

	// 2) Refresh memories. Visible enemies are tracked live; hidden ones keep their last known
	//    position and velocity until they've been out of sight for longer than the memory duration.
	const float MemoryDuration = Me->GetMemoryDuration();
	for (int32 Index = Memories.Num() - 1; Index >= 0; --Index)
	{
		FTargetMemory& Memory = Memories[Index];
		AArenaCharacter* Other = Memory.Actor.Get();
		if (!Other || !Other->IsAlive())
		{
			Memories.RemoveAtSwap(Index);
			continue;
		}

		if (Seen.Contains(Other))
		{
			Memory.bVisible = true;
			Memory.LastKnownLocation = Other->GetActorLocation();
			Memory.LastKnownVelocity = Other->GetVelocity();
			Memory.LastSeenTime = Now;
		}
		else
		{
			Memory.bVisible = false;
			if (Now - Memory.LastSeenTime > MemoryDuration)
			{
				Memories.RemoveAtSwap(Index);
			}
		}
	}

	// 3) Best visible target: closest, with bonuses for whoever just shot us and for our current target.
	AArenaCharacter* BestTarget = nullptr;
	float BestScore = -MAX_flt;
	const FTargetMemory* MostRecentHidden = nullptr;
	for (const FTargetMemory& Memory : Memories)
	{
		AArenaCharacter* Other = Memory.Actor.Get();
		if (Memory.bVisible)
		{
			float Score = -FVector::Dist(MyLocation, Other->GetActorLocation());
			if (Now - Memory.LastDamagedMeTime < 5.f)
			{
				Score += AttackerPriorityBonus;
			}
			if (Other == CurrentTarget.Get())
			{
				Score += TargetStickiness;
			}
			if (Score > BestScore)
			{
				BestScore = Score;
				BestTarget = Other;
			}
		}
		else if (!MostRecentHidden || Memory.LastSeenTime > MostRecentHidden->LastSeenTime)
		{
			MostRecentHidden = &Memory;
		}
	}

	// 4) Pick a state.
	const float FleeFraction = Me->GetFleeHealthFraction();
	const bool bThreatened = BestTarget != nullptr || (Now - LastDamagedTime) < 4.f;
	const bool bWantsToFlee = Now < FleeUntil || (FleeFraction > 0.f && Me->GetHealthFraction() <= FleeFraction && bThreatened);

	EArenaAIState NewState = EArenaAIState::Patrol;
	if (bWantsToFlee)
	{
		NewState = EArenaAIState::Flee;
	}
	else if (BestTarget)
	{
		NewState = EArenaAIState::Combat;
	}
	else if (MostRecentHidden)
	{
		NewState = EArenaAIState::Search;
	}
	else if (Now < InvestigateUntil)
	{
		NewState = EArenaAIState::Investigate;
	}

	if (BestTarget)
	{
		ThreatLocation = BestTarget->GetActorLocation();
	}
	else if (MostRecentHidden)
	{
		ThreatLocation = MostRecentHidden->LastKnownLocation;
	}
	if (MostRecentHidden)
	{
		SearchLocation = PredictLocation(*MostRecentHidden);
	}

	if (NewState != State)
	{
		// Drop any "look at that noise / shot" focus from the previous state.
		ClearFocus(EAIFocusPriority::Gameplay);
		State = NewState;
	}

	SetCurrentTarget(State == EArenaAIState::Combat ? BestTarget : nullptr, Now);
	UpdateFiring(Now);
	UpdateBlackboard();
}

void AArenaAIController::SetCurrentTarget(AArenaCharacter* NewTarget, float Now)
{
	if (NewTarget == CurrentTarget.Get())
	{
		return;
	}
	CurrentTarget = NewTarget;
	TargetAcquiredTime = Now;

	if (NewTarget)
	{
		SetFocus(NewTarget, ArenaFocusPriority::Target);
	}
	else
	{
		ClearFocus(ArenaFocusPriority::Target);
	}
}

void AArenaAIController::UpdateFiring(float Now)
{
	AArenaAICharacter* Me = GetArenaPawn();
	const AArenaCharacter* Target = CurrentTarget.Get();

	bool bShouldFire = State == EArenaAIState::Combat && Target && Target->IsAlive()
		&& (Now - TargetAcquiredTime) >= Me->GetReactionTime();

	if (bShouldFire)
	{
		// Only shoot once we've actually turned to face the target.
		const FVector ToTarget = (Target->GetActorLocation() - Me->GetActorLocation()).GetSafeNormal2D();
		const FVector Facing = Me->GetActorForwardVector().GetSafeNormal2D();
		bShouldFire = FVector::DotProduct(Facing, ToTarget) >= FMath::Cos(FMath::DegreesToRadians(FireConeDegrees));
	}

	if (bShouldFire)
	{
		Me->StartFiring();
	}
	else
	{
		Me->StopFiring();
	}
}

void AArenaAIController::UpdateBlackboard()
{
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	const AArenaAICharacter* Me = GetArenaPawn();
	if (!BlackboardComp || !Me)
	{
		return;
	}

	BlackboardComp->SetValueAsEnum(ArenaBlackboardKeys::AIState, static_cast<uint8>(State));
	BlackboardComp->SetValueAsEnum(ArenaBlackboardKeys::Personality, static_cast<uint8>(Me->GetPersonality()));
	BlackboardComp->SetValueAsObject(ArenaBlackboardKeys::TargetActor, CurrentTarget.Get());
	BlackboardComp->SetValueAsVector(ArenaBlackboardKeys::LastKnownLocation, SearchLocation);
	BlackboardComp->SetValueAsVector(ArenaBlackboardKeys::InvestigateLocation, InvestigateLocation);
}

FVector AArenaAIController::PredictLocation(const FTargetMemory& Memory) const
{
	const FVector Guess = Memory.LastKnownLocation + Memory.LastKnownVelocity.GetClampedToMaxSize(800.f) * PredictionTime;

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation NavLocation;
		if (NavSystem->ProjectPointToNavigation(Guess, NavLocation, FVector(300.f, 300.f, 300.f)))
		{
			return NavLocation.Location;
		}
		if (NavSystem->ProjectPointToNavigation(Memory.LastKnownLocation, NavLocation, FVector(300.f, 300.f, 300.f)))
		{
			return NavLocation.Location;
		}
	}
	return Memory.LastKnownLocation;
}

AArenaAIController::FTargetMemory* AArenaAIController::FindMemory(const AArenaCharacter* Actor)
{
	return Memories.FindByPredicate([Actor](const FTargetMemory& Memory) { return Memory.Actor.Get() == Actor; });
}

AArenaAIController::FTargetMemory& AArenaAIController::FindOrAddMemory(AArenaCharacter* Actor, float Now)
{
	if (FTargetMemory* Existing = FindMemory(Actor))
	{
		return *Existing;
	}
	FTargetMemory& Memory = Memories.AddDefaulted_GetRef();
	Memory.Actor = Actor;
	Memory.LastKnownLocation = Actor->GetActorLocation();
	Memory.LastSeenTime = Now;
	return Memory;
}

// ==================================================================== Life cycle

void AArenaAIController::ResetBrain()
{
	Memories.Reset();
	CurrentTarget = nullptr;
	State = EArenaAIState::Patrol;
	InvestigateUntil = -1000.f;
	FleeUntil = -1000.f;
	LastDamagedTime = -1000.f;
	ClearFocus(EAIFocusPriority::Gameplay);
	ClearFocus(ArenaFocusPriority::Target);
}

void AArenaAIController::OnPawnDied()
{
	StopMovement();
	ResetBrain();
	UpdateBlackboard();
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Dead"));
	}
}

void AArenaAIController::OnPawnRespawned()
{
	ResetBrain();
	UpdateBlackboard();
	if (BrainComponent && !bMatchOver)
	{
		BrainComponent->RestartLogic();
	}
	Think();
}

void AArenaAIController::OnMatchEnded()
{
	bMatchOver = true;
	GetWorldTimerManager().ClearTimer(ThinkTimer);
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (AArenaAICharacter* Me = GetArenaPawn())
	{
		Me->StopFiring();
	}
	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Match over"));
	}
}
