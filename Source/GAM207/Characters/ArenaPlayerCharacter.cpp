#include "Characters/ArenaPlayerCharacter.h"
#include "Game/ArenaPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"

AArenaPlayerCharacter::AArenaPlayerCharacter()
{
	BaseMaxHealth = 100.f;

	Weapon.WeaponName = NSLOCTEXT("Arena", "PlayerRifle", "Assault Rifle");
	Weapon.Damage = 12.f;
	Weapon.FireInterval = 0.15f;
	Weapon.ProjectilesPerShot = 1;
	Weapon.SpreadDegrees = 0.75f;
	Weapon.ProjectileSpeed = 6000.f;
	Weapon.NoiseLoudness = 1.f;
	Weapon.NoiseRange = 4500.f;
}

void AArenaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Move_Forward"), this, &AArenaPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Move_Right"), this, &AArenaPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AArenaPlayerCharacter::Turn);
	PlayerInputComponent->BindAxis(TEXT("Look_Up"), this, &AArenaPlayerCharacter::LookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Pressed, this, &AArenaPlayerCharacter::FirePressed);
	PlayerInputComponent->BindAction(TEXT("Fire"), IE_Released, this, &AArenaPlayerCharacter::FireReleased);
	PlayerInputComponent->BindAction(TEXT("ShowScore"), IE_Pressed, this, &AArenaPlayerCharacter::ScoreboardPressed);
	PlayerInputComponent->BindAction(TEXT("ShowScore"), IE_Released, this, &AArenaPlayerCharacter::ScoreboardReleased);
}

void AArenaPlayerCharacter::InitializeCombatant()
{
	if (UCameraComponent* Camera = FindComponentByClass<UCameraComponent>())
	{
		CameraComponent = Camera;
		CameraParent = Camera->GetAttachParent();
		CameraSocket = Camera->GetAttachSocketName();
		CameraRelativeTransform = Camera->GetRelativeTransform();
	}
}

void AArenaPlayerCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void AArenaPlayerCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void AArenaPlayerCharacter::Turn(float Value)
{
	AddControllerYawInput(Value * LookSensitivity);
}

void AArenaPlayerCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value * LookSensitivity);
}

void AArenaPlayerCharacter::FirePressed()
{
	StartFiring();
}

void AArenaPlayerCharacter::FireReleased()
{
	StopFiring();
}

void AArenaPlayerCharacter::ScoreboardPressed()
{
	if (AArenaPlayerController* PC = Cast<AArenaPlayerController>(Controller))
	{
		PC->SetScoreboardVisible(true);
	}
}

void AArenaPlayerCharacter::ScoreboardReleased()
{
	if (AArenaPlayerController* PC = Cast<AArenaPlayerController>(Controller))
	{
		PC->SetScoreboardVisible(false);
	}
}

FVector AArenaPlayerCharacter::GetAimLocation(const FVector& MuzzleLocation) const
{
	// Shoot at whatever is under the crosshair, so bullets converge on the screen centre even though the muzzle is offset.
	FVector ViewLocation;
	FRotator ViewRotation;
	if (!Controller)
	{
		return Super::GetAimLocation(MuzzleLocation);
	}
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FVector ViewDirection = ViewRotation.Vector();
	const FVector TraceEnd = ViewLocation + ViewDirection * 20000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArenaPlayerAim), true, this);
	FHitResult Hit;
	FVector AimPoint = TraceEnd;
	if (GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, Params))
	{
		AimPoint = Hit.ImpactPoint;
	}

	// If the crosshair target is behind the muzzle (pressed against a wall), just fire straight ahead.
	if (FVector::DotProduct(AimPoint - MuzzleLocation, ViewDirection) <= 10.f)
	{
		return MuzzleLocation + ViewDirection * 1000.f;
	}
	return AimPoint;
}

void AArenaPlayerCharacter::OnDeath(AArenaCharacter* Killer)
{
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		DisableInput(PC);
	}
	if (USceneComponent* Camera = CameraComponent.Get())
	{
		// Leave the camera where the head was while the body ragdolls.
		Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}
}

void AArenaPlayerCharacter::OnRespawn()
{
	USceneComponent* Camera = CameraComponent.Get();
	USceneComponent* Parent = CameraParent.Get();
	if (Camera && Parent)
	{
		Camera->AttachToComponent(Parent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CameraSocket);
		Camera->SetRelativeTransform(CameraRelativeTransform);
	}
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		EnableInput(PC);
	}
}
