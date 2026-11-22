#include "Gameplay/ArenaProjectile.h"
#include "Characters/ArenaCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArenaProjectile::AArenaProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	InitialLifeSpan = 3.f;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(6.f);
	// PhysicsBody object type: walls and characters block it, but projectiles pass through each other
	// (shotgun pellets would otherwise collide on spawn), through ragdolls, and through pickup overlap boxes.
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_PhysicsBody);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Collision->SetGenerateOverlapEvents(false);
	Collision->CanCharacterStepUpOn = ECB_No;
	RootComponent = Collision;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Collision);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetRelativeScale3D(FVector(0.12f));
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProjectileMaterial(TEXT("/Game/Player/ProjectileMaterial.ProjectileMaterial"));
	if (ProjectileMaterial.Succeeded())
	{
		Mesh->SetMaterial(0, ProjectileMaterial.Object);
	}

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 4000.f;
	Movement->MaxSpeed = 4000.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.f;
}

void AArenaProjectile::InitProjectile(float InDamage, float InSpeed)
{
	Damage = InDamage;
	Movement->InitialSpeed = InSpeed;
	Movement->MaxSpeed = InSpeed;
}

void AArenaProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (APawn* Shooter = GetInstigator())
	{
		Collision->IgnoreActorWhenMoving(Shooter, true);
	}
	Collision->OnComponentHit.AddUniqueDynamic(this, &AArenaProjectile::HandleHit);
}

void AArenaProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!IsValid(this) || OtherActor == GetInstigator())
	{
		return;
	}

	AArenaCharacter* Victim = Cast<AArenaCharacter>(OtherActor);
	if (Victim && Damage > 0.f)
	{
		// Point damage carries the shot direction, which the victim uses for its ragdoll impulse.
		UGameplayStatics::ApplyPointDamage(Victim, Damage, GetActorForwardVector(), Hit, GetInstigatorController(), this, UDamageType::StaticClass());
	}

	BP_OnImpact(Hit, Victim != nullptr);
	Destroy();
}
