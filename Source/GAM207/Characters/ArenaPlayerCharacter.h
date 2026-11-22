#pragma once

#include "CoreMinimal.h"
#include "Characters/ArenaCharacter.h"
#include "ArenaPlayerCharacter.generated.h"

/**
 * The player's pawn. Reparent PlayerCharacter_BP to this.
 * Input uses the existing Project Settings > Input mappings (Move_Forward, Move_Right, Turn, Look_Up, Jump, Fire, ShowScore).
 */
UCLASS()
class GAM207_API AArenaPlayerCharacter : public AArenaCharacter
{
	GENERATED_BODY()

public:
	AArenaPlayerCharacter();

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditAnywhere, Category = "Arena|Input", meta = (ClampMin = "0.05"))
	float LookSensitivity = 1.f;

protected:
	virtual void InitializeCombatant() override;
	virtual FVector GetAimLocation(const FVector& MuzzleLocation) const override;
	virtual void OnDeath(AArenaCharacter* Killer) override;
	virtual void OnRespawn() override;

private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void FirePressed();
	void FireReleased();
	void ScoreboardPressed();
	void ScoreboardReleased();

	/** The camera is unhooked on death (so it doesn't ride the ragdoll) and put back on respawn. */
	TWeakObjectPtr<USceneComponent> CameraComponent;
	TWeakObjectPtr<USceneComponent> CameraParent;
	FName CameraSocket;
	FTransform CameraRelativeTransform;
};
