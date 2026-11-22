#include "Characters/ArenaCharacter.h"
#include "GAM207.h"
#include "Game/ArenaGameMode.h"
#include "Gameplay/ArenaProjectile.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AArenaCharacter::AArenaCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	MuzzlePoint = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzlePoint"));
	MuzzlePoint->SetupAttachment(GetCapsuleComponent());
	MuzzlePoint->SetRelativeLocation(FVector(60.f, 20.f, 40.f));

	// Sensible defaults so the bare C++ class works on its own. Blueprint children (or reparented Blueprints) override these.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinMesh(TEXT("/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin"));
	static ConstructorHelpers::FClassFinder<UAnimInstance> MannequinAnim(TEXT("/Game/AnimStarterPack/UE4ASP_HeroTPP_AnimBlueprint"));
	if (MannequinMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MannequinMesh.Object);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -88.f), FRotator(0.f, -90.f, 0.f));
	}
	if (MannequinAnim.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(MannequinAnim.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitReact1(TEXT("/Game/AnimStarterPack/Hit_React_1.Hit_React_1"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitReact2(TEXT("/Game/AnimStarterPack/Hit_React_2.Hit_React_2"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitReact3(TEXT("/Game/AnimStarterPack/Hit_React_3.Hit_React_3"));
	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> HitReact4(TEXT("/Game/AnimStarterPack/Hit_React_4.Hit_React_4"));
	for (UAnimSequenceBase* Anim : { HitReact1.Object, HitReact2.Object, HitReact3.Object, HitReact4.Object })
	{
		if (Anim)
		{
			HitReactAnimations.Add(Anim);
		}
	}

	DefaultProjectileClass = AArenaProjectile::StaticClass();
}

void AArenaCharacter::BeginPlay()
{
	Super::BeginPlay();

	BaseWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	MeshRelativeTransform = GetMesh()->GetRelativeTransform();
	MeshCollisionProfile = GetMesh()->GetCollisionProfileName();

	InitializeCombatant();

	CurrentMaxHealth = FMath::Max(1.f, BaseMaxHealth * MatchHealthMultiplier);
	Health = CurrentMaxHealth;

	if (AArenaGameMode* GameMode = GetArenaGameMode())
	{
		GameMode->RegisterCombatant(this);
	}
	PostCombatantRegistered();
	BroadcastHealth();
}

void AArenaCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

// ==================================================================== Health

float AArenaCharacter::GetHealthFraction() const
{
	return CurrentMaxHealth > 0.f ? Health / CurrentMaxHealth : 0.f;
}

bool AArenaCharacter::Heal(float Amount)
{
	if (bIsDead || Amount <= 0.f || Health >= CurrentMaxHealth)
	{
		return false;
	}
	Health = FMath::Min(Health + Amount, CurrentMaxHealth);
	BroadcastHealth();
	return true;
}

void AArenaCharacter::ModifyMaxHealth(float Delta, bool bAlsoChangeHealth)
{
	if (bIsDead)
	{
		return;
	}
	CurrentMaxHealth = FMath::Max(1.f, CurrentMaxHealth + Delta);
	if (bAlsoChangeHealth)
	{
		Health += Delta;
	}
	Health = FMath::Clamp(Health, 1.f, CurrentMaxHealth);
	BroadcastHealth();
}

void AArenaCharacter::SetMatchModifiers(float HealthMultiplier, float DamageMultiplier)
{
	const float Fraction = GetHealthFraction();
	MatchHealthMultiplier = FMath::Max(0.1f, HealthMultiplier);
	MatchDamageMultiplier = FMath::Max(0.f, DamageMultiplier);
	CurrentMaxHealth = FMath::Max(1.f, BaseMaxHealth * MatchHealthMultiplier);
	Health = bIsDead ? 0.f : FMath::Max(1.f, CurrentMaxHealth * Fraction);
	BroadcastHealth();
}

void AArenaCharacter::BroadcastHealth()
{
	OnHealthChanged.Broadcast(Health, CurrentMaxHealth);
}

AArenaCharacter* AArenaCharacter::ResolveAttacker(AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageCauser)
	{
		if (AArenaCharacter* Direct = Cast<AArenaCharacter>(DamageCauser))
		{
			return Direct;
		}
		if (AArenaCharacter* ByInstigator = Cast<AArenaCharacter>(DamageCauser->GetInstigator()))
		{
			return ByInstigator;
		}
		if (AArenaCharacter* ByOwner = Cast<AArenaCharacter>(DamageCauser->GetOwner()))
		{
			return ByOwner;
		}
	}
	return EventInstigator ? Cast<AArenaCharacter>(EventInstigator->GetPawn()) : nullptr;
}

float AArenaCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead || bFrozen || DamageAmount <= 0.f)
	{
		return 0.f;
	}

	const float Applied = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	if (Applied <= 0.f)
	{
		return 0.f;
	}

	AArenaCharacter* Attacker = ResolveAttacker(EventInstigator, DamageCauser);
	if (Attacker == this)
	{
		Attacker = nullptr;
	}

	FVector HitDirection = FVector::ZeroVector;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		HitDirection = static_cast<const FPointDamageEvent&>(DamageEvent).ShotDirection;
	}
	else if (Attacker)
	{
		HitDirection = (GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
	}

	Health = FMath::Max(0.f, Health - Applied);
	BroadcastHealth();

	// Tell this pawn's AI perception where the shot came from. AArenaAIController reacts to the
	// resulting Damage stimulus even when it never saw the attacker.
	if (Attacker)
	{
		UAISense_Damage::ReportDamageEvent(this, this, Attacker, Applied, Attacker->GetActorLocation(), GetActorLocation());
	}

	OnDamaged.Broadcast(Attacker, Applied, HitDirection);
	BP_OnTookDamage(Attacker, Applied, HitDirection);

	const bool bKilled = Health <= 0.f;
	if (Attacker)
	{
		Attacker->NotifyHitLanded(this, Applied, bKilled);
	}

	if (bKilled)
	{
		Die(Attacker, HitDirection);
	}
	else
	{
		PlayHitReact();
	}
	return Applied;
}

void AArenaCharacter::NotifyHitLanded(AArenaCharacter* Victim, float Damage, bool bKilled)
{
	if (AArenaGameMode* GameMode = GetArenaGameMode())
	{
		GameMode->RecordHit(this, Damage);
	}
	OnHitLanded.Broadcast(Victim, Damage, bKilled);
}

void AArenaCharacter::PlayHitReact()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (HitReactAnimations.Num() == 0 || Now - LastHitReactTime < HitReactCooldown)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimSequenceBase* Anim = HitReactAnimations[FMath::RandRange(0, HitReactAnimations.Num() - 1)];
	if (!AnimInstance || !Anim)
	{
		return;
	}

	if (UAnimMontage* Montage = Cast<UAnimMontage>(Anim))
	{
		AnimInstance->Montage_Play(Montage);
	}
	else
	{
		// Plays a plain animation sequence through the Slot node without needing a montage asset.
		AnimInstance->PlaySlotAnimationAsDynamicMontage(Anim, HitReactSlotName, 0.08f, 0.2f);
	}
	LastHitReactTime = Now;
}

// ==================================================================== Death / respawn

void AArenaCharacter::Die(AArenaCharacter* Killer, const FVector& HitDirection)
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	StopFiring();
	ClearBuffs();

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (bRagdollOnDeath && MeshComp && MeshComp->GetPhysicsAsset())
	{
		MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
		MeshComp->SetSimulatePhysics(true);
		MeshComp->WakeAllRigidBodies();
		if (!HitDirection.IsNearlyZero())
		{
			MeshComp->AddImpulseToAllBodiesBelow(HitDirection.GetSafeNormal() * DeathImpulse, NAME_None, true, true);
		}
	}

	OnDeath(Killer);
	OnDied.Broadcast(Killer);
	BP_OnDied(Killer);

	if (AArenaGameMode* GameMode = GetArenaGameMode())
	{
		GameMode->HandleCombatantDeath(this, Killer);
	}
}

void AArenaCharacter::RespawnAt(const FTransform& SpawnTransform)
{
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		if (MeshComp->IsSimulatingPhysics())
		{
			MeshComp->SetSimulatePhysics(false);
		}
		MeshComp->SetCollisionProfileName(MeshCollisionProfile);
		MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		MeshComp->SetRelativeTransform(MeshRelativeTransform);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	SetActorLocationAndRotation(SpawnTransform.GetLocation(), SpawnTransform.GetRotation(), false, nullptr, ETeleportType::ResetPhysics);
	if (Controller)
	{
		Controller->SetControlRotation(SpawnTransform.Rotator());
	}

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->StopMovementImmediately();
	Movement->SetMovementMode(MOVE_Walking);

	bIsDead = false;
	RespawnAtTime = 0.f;
	LastFireTime = -1000.f;
	CurrentMaxHealth = FMath::Max(1.f, BaseMaxHealth * MatchHealthMultiplier);
	Health = CurrentMaxHealth;
	UpdateWalkSpeed();
	BroadcastHealth();

	OnRespawn();
	OnRespawned.Broadcast();
	BP_OnRespawned();
}

float AArenaCharacter::GetRespawnTimeRemaining() const
{
	if (!bIsDead || RespawnAtTime <= 0.f)
	{
		return 0.f;
	}
	return FMath::Max(0.f, RespawnAtTime - GetWorld()->GetTimeSeconds());
}

void AArenaCharacter::FreezeForMatchEnd()
{
	bFrozen = true;
	StopFiring();
	if (!bIsDead)
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}
}

AArenaGameMode* AArenaCharacter::GetArenaGameMode() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetAuthGameMode<AArenaGameMode>() : nullptr;
}

// ==================================================================== Weapon

void AArenaCharacter::StartFiring()
{
	if (bIsDead || bFrozen)
	{
		return;
	}

	FTimerManager& Timers = GetWorldTimerManager();
	if (Timers.IsTimerActive(FireTimer))
	{
		return;
	}

	const float Interval = FMath::Max(0.05f, Weapon.FireInterval);
	const float TimeUntilReady = (LastFireTime + Interval) - GetWorld()->GetTimeSeconds();
	if (TimeUntilReady <= 0.f)
	{
		FireShot();
		Timers.SetTimer(FireTimer, this, &AArenaCharacter::FireShot, Interval, true);
	}
	else
	{
		Timers.SetTimer(FireTimer, this, &AArenaCharacter::FireShot, Interval, true, TimeUntilReady);
	}
}

void AArenaCharacter::StopFiring()
{
	GetWorldTimerManager().ClearTimer(FireTimer);
}

bool AArenaCharacter::IsFiring() const
{
	return GetWorldTimerManager().IsTimerActive(FireTimer);
}

FVector AArenaCharacter::GetAimLocation(const FVector& MuzzleLocation) const
{
	return MuzzleLocation + GetBaseAimRotation().Vector() * 10000.f;
}

void AArenaCharacter::FireShot()
{
	if (bIsDead || bFrozen)
	{
		StopFiring();
		return;
	}

	UWorld* World = GetWorld();
	TSubclassOf<AArenaProjectile> ProjectileClass = Weapon.ProjectileClass ? Weapon.ProjectileClass : DefaultProjectileClass;
	if (!ProjectileClass)
	{
		ProjectileClass = AArenaProjectile::StaticClass();
	}

	const FVector MuzzleLocation = MuzzlePoint->GetComponentLocation();
	FVector AimDirection = (GetAimLocation(MuzzleLocation) - MuzzleLocation).GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector();
	}

	const float SpreadRadians = FMath::DegreesToRadians(Weapon.SpreadDegrees);
	const float ShotDamage = Weapon.Damage * MatchDamageMultiplier * DamageBoostMultiplier;
	const int32 ProjectileCount = FMath::Max(1, Weapon.ProjectilesPerShot);

	for (int32 Index = 0; Index < ProjectileCount; ++Index)
	{
		const FVector Direction = SpreadRadians > 0.f ? FMath::VRandCone(AimDirection, SpreadRadians) : AimDirection;
		const FTransform SpawnTransform(Direction.Rotation(), MuzzleLocation);

		AArenaProjectile* Projectile = World->SpawnActorDeferred<AArenaProjectile>(ProjectileClass, SpawnTransform, this, this,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Projectile)
		{
			Projectile->InitProjectile(ShotDamage, Weapon.ProjectileSpeed);
			Projectile->FinishSpawning(SpawnTransform);
		}
	}

	LastFireTime = World->GetTimeSeconds();

	if (AArenaGameMode* GameMode = GetArenaGameMode())
	{
		GameMode->RecordShotsFired(this, ProjectileCount);
	}

	// Every gunshot is a sound the AI can hear (AArenaAIController's hearing sense picks it up).
	UAISense_Hearing::ReportNoiseEvent(this, MuzzleLocation, Weapon.NoiseLoudness, this, Weapon.NoiseRange, TEXT("Gunshot"));

	BP_OnWeaponFired(MuzzleLocation, AimDirection);
}

// ==================================================================== Buffs

void AArenaCharacter::ApplyDamageBoost(float Multiplier, float Duration)
{
	if (bIsDead)
	{
		return;
	}
	DamageBoostMultiplier = FMath::Max(0.f, Multiplier);
	GetWorldTimerManager().SetTimer(DamageBoostTimer, this, &AArenaCharacter::EndDamageBoost, FMath::Max(0.1f, Duration), false);
	OnBuffsChanged.Broadcast();
}

void AArenaCharacter::ApplySpeedBoost(float Multiplier, float Duration)
{
	if (bIsDead)
	{
		return;
	}
	SpeedBoostMultiplier = FMath::Max(0.1f, Multiplier);
	UpdateWalkSpeed();
	GetWorldTimerManager().SetTimer(SpeedBoostTimer, this, &AArenaCharacter::EndSpeedBoost, FMath::Max(0.1f, Duration), false);
	OnBuffsChanged.Broadcast();
}

float AArenaCharacter::GetDamageBoostTimeRemaining() const
{
	return GetWorldTimerManager().IsTimerActive(DamageBoostTimer) ? GetWorldTimerManager().GetTimerRemaining(DamageBoostTimer) : 0.f;
}

float AArenaCharacter::GetSpeedBoostTimeRemaining() const
{
	return GetWorldTimerManager().IsTimerActive(SpeedBoostTimer) ? GetWorldTimerManager().GetTimerRemaining(SpeedBoostTimer) : 0.f;
}

void AArenaCharacter::EndDamageBoost()
{
	DamageBoostMultiplier = 1.f;
	OnBuffsChanged.Broadcast();
}

void AArenaCharacter::EndSpeedBoost()
{
	SpeedBoostMultiplier = 1.f;
	UpdateWalkSpeed();
	OnBuffsChanged.Broadcast();
}

void AArenaCharacter::ClearBuffs()
{
	GetWorldTimerManager().ClearTimer(DamageBoostTimer);
	GetWorldTimerManager().ClearTimer(SpeedBoostTimer);
	DamageBoostMultiplier = 1.f;
	SpeedBoostMultiplier = 1.f;
	UpdateWalkSpeed();
	OnBuffsChanged.Broadcast();
}

void AArenaCharacter::UpdateWalkSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * SpeedBoostMultiplier;
}
