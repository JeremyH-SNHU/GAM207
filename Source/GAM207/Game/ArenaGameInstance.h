#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ArenaTypes.h"
#include "ArenaGameInstance.generated.h"

class UArenaEnemyArchetype;

/**
 * Survives level loads, so it carries the match settings from the main menu into the arena and back.
 * Set in Project Settings > Maps & Modes > Game Instance Class (already done in DefaultEngine.ini).
 */
UCLASS()
class GAM207_API UArenaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UArenaGameInstance();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena")
	FArenaMatchSettings MatchSettings;

	/** Enemy types to use. Leave empty to use every ArenaEnemyArchetype Data Asset in the project (or the built-in five if there are none). */
	UPROPERTY(EditDefaultsOnly, Category = "Arena")
	TArray<TObjectPtr<UArenaEnemyArchetype>> Archetypes;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Levels")
	TSoftObjectPtr<UWorld> ArenaLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Arena|Levels")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	/** Set by the end screen's "Match Settings" button so the menu opens straight to the setup page. */
	UPROPERTY(BlueprintReadWrite, Category = "Arena")
	bool bOpenMatchSetupOnMenuLoad = false;

	UFUNCTION(BlueprintCallable, Category = "Arena")
	TArray<UArenaEnemyArchetype*> GetArchetypes();

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void StartMatch();

	/** Opens the main menu map. bOpenMatchSetup jumps straight to the match settings page. */
	UFUNCTION(BlueprintCallable, Category = "Arena")
	void OpenMainMenu(bool bOpenMatchSetup);

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void ResetMatchSettings();

	UFUNCTION(BlueprintCallable, Category = "Arena")
	void QuitGame();

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UArenaEnemyArchetype>> ResolvedArchetypes;
};
