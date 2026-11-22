#include "Gameplay/ArenaPickup.h"
#include "Characters/ArenaCharacter.h"
#include "TimerManager.h"

AArenaPickup::AArenaPickup()
{
	PrimaryActorTick.bCanEverTick = false;
}

bool AArenaPickup::TryApplyTo(AArenaCharacter* Character)
{
	if (!IsAvailable() || !Character || !Character->IsAlive())
	{
		return false;
	}

	bool bApplied = false;
	switch (PickupType)
	{
	case EArenaPickupType::Heal:
		if (bOnlyWhenNeeded && Character->GetHealth() >= Character->GetMaxHealth())
		{
			return false;
		}
		bApplied = Character->Heal(Amount);
		break;

	case EArenaPickupType::MaxHealth:
		Character->ModifyMaxHealth(Amount, true);
		bApplied = true;
		break;

	case EArenaPickupType::DamageBoost:
		Character->ApplyDamageBoost(Amount, Duration);
		bApplied = true;
		break;

	case EArenaPickupType::SpeedBoost:
		Character->ApplySpeedBoost(Amount, Duration);
		bApplied = true;
		break;
	}

	if (bApplied)
	{
		BP_OnPickedUp(Character);
		Consume();
	}
	return bApplied;
}

void AArenaPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	TryApplyTo(Cast<AArenaCharacter>(OtherActor));
}

void AArenaPickup::SetPickupEnabled(bool bEnabled)
{
	bDisabledForMatch = !bEnabled;
	SetShown(bEnabled && bAvailable);
}

void AArenaPickup::Consume()
{
	bAvailable = false;
	SetShown(false);

	if (RespawnTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, this, &AArenaPickup::Respawn, RespawnTime, false);
	}
}

void AArenaPickup::Respawn()
{
	bAvailable = true;
	if (!bDisabledForMatch)
	{
		SetShown(true);
		BP_OnRespawned();
	}
}

void AArenaPickup::SetShown(bool bShown)
{
	SetActorHiddenInGame(!bShown);
	SetActorEnableCollision(bShown);
}
