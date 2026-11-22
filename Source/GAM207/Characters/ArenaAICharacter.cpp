#include "Characters/ArenaAICharacter.h"
#include "AI/ArenaAIController.h"
#include "Gameplay/ArenaEnemyArchetype.h"
#include "UI/ArenaOverheadWidget.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AArenaAICharacter::AArenaAICharacter()
{
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AArenaAIController::StaticClass();

	// Face the controller's focus (the target) while moving, so enemies can strafe and shoot.
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = false;
		Movement->bUseControllerDesiredRotation = true;
		Movement->RotationRate = FRotator(0.f, 540.f, 0.f);
	}
}

EArenaPersonality AArenaAICharacter::GetPersonality() const
{
	return Archetype ? Archetype->Personality : FallbackPersonality;
}

float AArenaAICharacter::GetPreferredRange() const
{
	return Archetype ? Archetype->PreferredRange : 1000.f;
}

float AArenaAICharacter::GetFleeHealthFraction() const
{
	if (Archetype)
	{
		return Archetype->FleeHealthFraction;
	}
	return FallbackPersonality == EArenaPersonality::Cowardly ? 0.4f : 0.f;
}

float AArenaAICharacter::GetMemoryDuration() const
{
	return Archetype ? Archetype->MemoryDuration : 10.f;
}

float AArenaAICharacter::GetReactionTime() const
{
	return Archetype ? Archetype->ReactionTime : 0.35f;
}

void AArenaAICharacter::InitializeCombatant()
{
	if (!Archetype)
	{
		return;
	}

	BaseMaxHealth = Archetype->MaxHealth;
	BaseWalkSpeed = Archetype->WalkSpeed;
	UpdateWalkSpeed();
	Weapon = Archetype->Weapon;

	if (!FMath::IsNearlyEqual(Archetype->BodyScale, 1.f))
	{
		// Scale the mesh around its feet rather than the whole actor, so the capsule still fits the navmesh.
		MeshRelativeTransform.SetScale3D(MeshRelativeTransform.GetScale3D() * Archetype->BodyScale);
		GetMesh()->SetRelativeScale3D(MeshRelativeTransform.GetScale3D());
	}
	ApplyBodyColor(Archetype->BodyColor);
}

void AArenaAICharacter::PostCombatantRegistered()
{
	TArray<UWidgetComponent*> WidgetComponents;
	GetComponents(WidgetComponents);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (UArenaOverheadWidget* Overhead = Cast<UArenaOverheadWidget>(WidgetComponent->GetUserWidgetObject()))
		{
			OverheadWidgetComponent = WidgetComponent;
			Overhead->SetCombatant(this);
		}
	}
}

void AArenaAICharacter::ApplyBodyColor(const FLinearColor& Color)
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}
	// Every material slot gets the parameter; slots without a "BodyColor" parameter simply ignore it.
	for (int32 Index = 0; Index < MeshComp->GetNumMaterials(); ++Index)
	{
		if (UMaterialInstanceDynamic* Material = MeshComp->CreateDynamicMaterialInstance(Index))
		{
			Material->SetVectorParameterValue(BodyColorParameter, Color);
		}
	}
}

FVector AArenaAICharacter::GetAimLocation(const FVector& MuzzleLocation) const
{
	const AArenaAIController* AIController = Cast<AArenaAIController>(GetController());
	const AArenaCharacter* Target = AIController ? AIController->GetCurrentTarget() : nullptr;
	if (!Target)
	{
		return MuzzleLocation + GetActorForwardVector() * 5000.f;
	}

	// Lead moving targets a little so strafing isn't a perfect defence.
	FVector TargetLocation = Target->GetActorLocation();
	const float TravelTime = FVector::Dist(MuzzleLocation, TargetLocation) / FMath::Max(100.f, Weapon.ProjectileSpeed);
	TargetLocation += Target->GetVelocity() * TravelTime * AimLeadFactor;
	return TargetLocation;
}

void AArenaAICharacter::OnDeath(AArenaCharacter* Killer)
{
	if (UWidgetComponent* Overhead = OverheadWidgetComponent.Get())
	{
		Overhead->SetVisibility(false);
	}
	if (AArenaAIController* AIController = Cast<AArenaAIController>(GetController()))
	{
		AIController->OnPawnDied();
	}
}

void AArenaAICharacter::OnRespawn()
{
	if (UWidgetComponent* Overhead = OverheadWidgetComponent.Get())
	{
		Overhead->SetVisibility(true);
	}
	if (AArenaAIController* AIController = Cast<AArenaAIController>(GetController()))
	{
		AIController->OnPawnRespawned();
	}
}
