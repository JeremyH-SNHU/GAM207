#include "Game/ArenaGameInstance.h"
#include "GAM207.h"
#include "Gameplay/ArenaEnemyArchetype.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/PackageName.h"

UArenaGameInstance::UArenaGameInstance()
{
	ArenaLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/Level_1.Level_1")));
	MainMenuLevel = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Map/MainMenu.MainMenu")));
}

TArray<UArenaEnemyArchetype*> UArenaGameInstance::GetArchetypes()
{
	if (ResolvedArchetypes.Num() == 0)
	{
		for (UArenaEnemyArchetype* Archetype : Archetypes)
		{
			if (Archetype)
			{
				ResolvedArchetypes.Add(Archetype);
			}
		}
	}

	if (ResolvedArchetypes.Num() == 0)
	{
		// No list set: use every archetype Data Asset in the project, in name order.
		if (IAssetRegistry* AssetRegistry = IAssetRegistry::Get())
		{
			TArray<FAssetData> Assets;
			AssetRegistry->GetAssetsByClass(UArenaEnemyArchetype::StaticClass()->GetClassPathName(), Assets, true);
			Assets.Sort([](const FAssetData& A, const FAssetData& B) { return A.AssetName.LexicalLess(B.AssetName); });
			for (const FAssetData& Asset : Assets)
			{
				if (UArenaEnemyArchetype* Archetype = Cast<UArenaEnemyArchetype>(Asset.GetAsset()))
				{
					ResolvedArchetypes.Add(Archetype);
				}
			}
		}
	}

	if (ResolvedArchetypes.Num() == 0)
	{
		UArenaEnemyArchetype::CreateBuiltInArchetypes(this, ResolvedArchetypes);
	}

	TArray<UArenaEnemyArchetype*> Result;
	for (UArenaEnemyArchetype* Archetype : ResolvedArchetypes)
	{
		Result.Add(Archetype);
	}
	return Result;
}

void UArenaGameInstance::StartMatch()
{
	bOpenMatchSetupOnMenuLoad = false;
	if (ArenaLevel.IsNull())
	{
		UE_LOG(LogArena, Error, TEXT("ArenaGameInstance: Arena Level is not set."));
		return;
	}
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, ArenaLevel);
}

void UArenaGameInstance::OpenMainMenu(bool bOpenMatchSetup)
{
	const FString MenuPackage = MainMenuLevel.ToSoftObjectPath().GetLongPackageName();
	if (MainMenuLevel.IsNull() || !FPackageName::DoesPackageExist(MenuPackage))
	{
		UE_LOG(LogArena, Warning, TEXT("Main menu level '%s' doesn't exist yet; restarting the arena instead."), *MenuPackage);
		StartMatch();
		return;
	}
	bOpenMatchSetupOnMenuLoad = bOpenMatchSetup;
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, MainMenuLevel);
}

void UArenaGameInstance::ResetMatchSettings()
{
	MatchSettings = FArenaMatchSettings();
}

void UArenaGameInstance::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetFirstLocalPlayerController(), EQuitPreference::Quit, false);
}
