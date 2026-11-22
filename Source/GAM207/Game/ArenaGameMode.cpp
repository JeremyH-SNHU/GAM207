#include "Game/ArenaGameMode.h"
#include "GAM207.h"
#include "AI/ArenaAIController.h"
#include "Characters/ArenaAICharacter.h"
#include "Characters/ArenaPlayerCharacter.h"
#include "Game/ArenaGameInstance.h"
#include "Game/ArenaPlayerController.h"
#include "Gameplay/ArenaEnemyArchetype.h"
#include "Gameplay/ArenaPickup.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AArenaGameMode::AArenaGameMode()
{
	DefaultPawnClass = AArenaPlayerCharacter::StaticClass();
	PlayerControllerClass = AArenaPlayerController::StaticClass();

	EnemyClass = TSoftClassPtr<AArenaAICharacter>(FSoftObjectPath(TEXT("/Game/AI/AICharacter_BP.AICharacter_BP_C")));
	EnemyControllerClass = AArenaAIController::StaticClass();
	SpawnPointClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Spawner/SpawnPoint.SpawnPoint_C")));
	LegacyPickupClass = TSoftClassPtr<AActor>(FSoftObjectPath(TEXT("/Game/Pickups/Pickup_Master.Pickup_Master_C")));
}

void AArenaGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (UArenaGameInstance* GameInstance = GetGameInstance<UArenaGameInstance>())
	{
		Settings = GameInstance->MatchSettings;
		for (UArenaEnemyArchetype* Archetype : GameInstance->GetArchetypes())
		{
			Archetypes.Add(Archetype);
		}
	}
	else
	{
		UE_LOG(LogArena, Warning, TEXT("Game Instance is not an ArenaGameInstance; using default match settings. Check Project Settings > Maps & Modes > Game Instance Class."));
	}

	if (Archetypes.Num() == 0)
	{
		UArenaEnemyArchetype::CreateBuiltInArchetypes(this, Archetypes);
	}
	ArchetypeOffset = FMath::RandRange(0, FMath::Max(0, Archetypes.Num() - 1));
}

void AArenaGameMode::StartPlay()
{
	// Runs before any actor's BeginPlay, so hand-placed enemies are gone before they register.
	if (bReplacePlacedEnemies)
	{
		RemovePlacedEnemies();
	}
	CacheSpawnPoints();

	Super::StartPlay();

	MatchStartTime = GetWorld()->GetTimeSeconds();
	ApplyPickupSetting();
	SpawnEnemies();

	if (Settings.TimeLimitMinutes > 0.f)
	{
		GetWorldTimerManager().SetTimer(TimeLimitTimer, this, &AArenaGameMode::HandleTimeLimitReached, Settings.TimeLimitMinutes * 60.f, false);
	}
	OnScoresChanged.Broadcast();
}

// ==================================================================== Setup

UClass* AArenaGameMode::ResolveEnemyClass() const
{
	UClass* Loaded = EnemyClass.LoadSynchronous();
	if (Loaded && Loaded->IsChildOf(AArenaAICharacter::StaticClass()))
	{
		return Loaded;
	}

	UE_LOG(LogArena, Warning, TEXT("Enemy Class '%s' is missing or not an ArenaAICharacter (reparent AICharacter_BP). Spawning the plain C++ enemy instead."), *EnemyClass.ToString());
	return AArenaAICharacter::StaticClass();
}

void AArenaGameMode::RemovePlacedEnemies()
{
	TArray<AArenaAICharacter*> Placed;
	for (TActorIterator<AArenaAICharacter> It(GetWorld()); It; ++It)
	{
		Placed.Add(*It);
	}
	for (AArenaAICharacter* Enemy : Placed)
	{
		if (AController* EnemyController = Enemy->GetController())
		{
			EnemyController->UnPossess();
			EnemyController->Destroy();
		}
		Enemy->Destroy();
	}
}

void AArenaGameMode::CacheSpawnPoints()
{
	SpawnPoints.Reset();
	if (UClass* PointClass = SpawnPointClass.LoadSynchronous())
	{
		for (TActorIterator<AActor> It(GetWorld(), PointClass); It; ++It)
		{
			SpawnPoints.Add(*It);
		}
	}
	if (SpawnPoints.Num() == 0)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			SpawnPoints.Add(*It);
		}
	}
	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogArena, Warning, TEXT("No spawn points or Player Starts found; everyone will spawn at the world origin."));
	}
}

void AArenaGameMode::SpawnEnemies()
{
	UClass* Class = ResolveEnemyClass();
	UClass* ControllerClass = EnemyControllerClass ? EnemyControllerClass.Get() : AArenaAIController::StaticClass();

	for (int32 Index = 0; Index < Settings.EnemyCount; ++Index)
	{
		const FTransform SpawnTransform = ChooseSpawnTransform(nullptr);
		AArenaAICharacter* Enemy = GetWorld()->SpawnActorDeferred<AArenaAICharacter>(Class, SpawnTransform, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Enemy)
		{
			continue;
		}
		// Set before FinishSpawning: auto-possession and BeginPlay both happen inside it.
		Enemy->AIControllerClass = ControllerClass;
		Enemy->AutoPossessAI = EAutoPossessAI::Spawned;
		Enemy->SetArchetype(PickArchetype(Index));
		Enemy->FinishSpawning(SpawnTransform);
	}
}

UArenaEnemyArchetype* AArenaGameMode::PickArchetype(int32 SpawnIndex) const
{
	if (Archetypes.Num() == 0)
	{
		return nullptr;
	}
	if (Archetypes.IsValidIndex(Settings.EnemyArchetypeIndex))
	{
		return Archetypes[Settings.EnemyArchetypeIndex];
	}
	// Mixed: walk the list from a random start so every type shows up once there are enough enemies.
	return Archetypes[(SpawnIndex + ArchetypeOffset) % Archetypes.Num()];
}

void AArenaGameMode::ApplyPickupSetting()
{
	if (Settings.bPickupsEnabled)
	{
		return;
	}

	for (TActorIterator<AArenaPickup> It(GetWorld()); It; ++It)
	{
		It->SetPickupEnabled(false);
	}

	if (UClass* LegacyClass = LegacyPickupClass.LoadSynchronous())
	{
		TArray<AActor*> Legacy;
		for (TActorIterator<AActor> It(GetWorld(), LegacyClass); It; ++It)
		{
			if (!It->IsA<AArenaPickup>())
			{
				Legacy.Add(*It);
			}
		}
		for (AActor* Pickup : Legacy)
		{
			Pickup->Destroy();
		}
	}
}

FTransform AArenaGameMode::ChooseSpawnTransform(const AArenaCharacter* ForCombatant) const
{
	TArray<TPair<float, AActor*>> Scored;
	for (const TWeakObjectPtr<AActor>& PointPtr : SpawnPoints)
	{
		AActor* Point = PointPtr.Get();
		if (!Point)
		{
			continue;
		}
		// Score each point by how far it is from the nearest living combatant.
		float Nearest = MAX_flt;
		for (const FArenaCombatantStats& Stats : Combatants)
		{
			const AArenaCharacter* Other = Stats.Combatant;
			if (Other && Other != ForCombatant && Other->IsAlive())
			{
				Nearest = FMath::Min(Nearest, FVector::Dist(Point->GetActorLocation(), Other->GetActorLocation()));
			}
		}
		Scored.Emplace(Nearest, Point);
	}

	if (Scored.Num() == 0)
	{
		return FTransform(FRotator::ZeroRotator, FVector(0.f, 0.f, 200.f));
	}

	// Pick randomly among the safest third so spawns stay unpredictable.
	Scored.Sort([](const TPair<float, AActor*>& A, const TPair<float, AActor*>& B) { return A.Key > B.Key; });
	const int32 Choices = FMath::Max(1, FMath::CeilToInt(Scored.Num() / 3.f));
	const AActor* Chosen = Scored[FMath::RandRange(0, Choices - 1)].Value;
	return FTransform(FRotator(0.f, Chosen->GetActorRotation().Yaw, 0.f), Chosen->GetActorLocation());
}

// ==================================================================== Combatants

FArenaCombatantStats* AArenaGameMode::FindStats(const AArenaCharacter* Combatant)
{
	return Combatants.FindByPredicate([Combatant](const FArenaCombatantStats& Stats) { return Stats.Combatant == Combatant; });
}

const FArenaCombatantStats* AArenaGameMode::FindStats(const AArenaCharacter* Combatant) const
{
	return Combatants.FindByPredicate([Combatant](const FArenaCombatantStats& Stats) { return Stats.Combatant == Combatant; });
}

void AArenaGameMode::RegisterCombatant(AArenaCharacter* Combatant)
{
	if (!Combatant || FindStats(Combatant))
	{
		return;
	}

	FArenaCombatantStats& Stats = Combatants.AddDefaulted_GetRef();
	Stats.Combatant = Combatant;
	Stats.bIsPlayer = Combatant->IsA<AArenaPlayerCharacter>();

	if (Stats.bIsPlayer)
	{
		Stats.DisplayName = NSLOCTEXT("Arena", "PlayerName", "You");
		Combatant->SetMatchModifiers(Settings.PlayerHealthMultiplier, Settings.PlayerDamageMultiplier);
	}
	else
	{
		const AArenaAICharacter* Enemy = Cast<AArenaAICharacter>(Combatant);
		const FText BaseName = (Enemy && Enemy->GetArchetype()) ? Enemy->GetArchetype()->DisplayName : NSLOCTEXT("Arena", "EnemyName", "Enemy");
		int32& Count = NameCounts.FindOrAdd(BaseName.ToString());
		++Count;
		Stats.DisplayName = FText::Format(NSLOCTEXT("Arena", "EnemyNameFormat", "{0} {1}"), BaseName, FText::AsNumber(Count));
		Combatant->SetMatchModifiers(Settings.EnemyHealthMultiplier, Settings.EnemyDamageMultiplier);
	}

	Combatant->SetCombatantName(Stats.DisplayName);
	OnScoresChanged.Broadcast();
}

void AArenaGameMode::HandleCombatantDeath(AArenaCharacter* Victim, AArenaCharacter* Killer)
{
	if (!Victim || bMatchOver)
	{
		return;
	}

	if (FArenaCombatantStats* VictimStats = FindStats(Victim))
	{
		++VictimStats->Deaths;
	}

	int32 Points = 0;
	int32 KillerScore = 0;
	if (Killer && Killer != Victim)
	{
		if (FArenaCombatantStats* KillerStats = FindStats(Killer))
		{
			++KillerStats->Kills;
			Points = Settings.PointsPerKill;
			KillerStats->Score += Points;
			KillerScore = KillerStats->Score;
		}
	}

	OnKill.Broadcast(Killer, Victim, Points);
	OnScoresChanged.Broadcast();

	if (Settings.ScoreLimit > 0 && KillerScore >= Settings.ScoreLimit)
	{
		EndMatch(EArenaMatchEndReason::ScoreLimit);
		return;
	}

	const float Delay = FMath::Max(0.5f, Settings.RespawnDelay);
	Victim->SetRespawnTime(GetWorld()->GetTimeSeconds() + Delay);
	FTimerHandle RespawnTimer;
	GetWorldTimerManager().SetTimer(RespawnTimer,
		FTimerDelegate::CreateUObject(this, &AArenaGameMode::RespawnCombatant, TWeakObjectPtr<AArenaCharacter>(Victim)), Delay, false);
}

void AArenaGameMode::RespawnCombatant(TWeakObjectPtr<AArenaCharacter> Combatant)
{
	AArenaCharacter* Character = Combatant.Get();
	if (bMatchOver || !Character || Character->IsAlive())
	{
		return;
	}
	Character->RespawnAt(ChooseSpawnTransform(Character));
}

void AArenaGameMode::RecordShotsFired(AArenaCharacter* Shooter, int32 Count)
{
	if (FArenaCombatantStats* Stats = FindStats(Shooter))
	{
		Stats->ShotsFired += Count;
	}
}

void AArenaGameMode::RecordHit(AArenaCharacter* Shooter, float Damage)
{
	if (FArenaCombatantStats* Stats = FindStats(Shooter))
	{
		++Stats->ShotsHit;
		Stats->DamageDealt += Damage;
	}
}

// ==================================================================== Queries

TArray<FArenaCombatantStats> AArenaGameMode::GetSortedStats() const
{
	TArray<FArenaCombatantStats> Sorted;
	for (const FArenaCombatantStats& Stats : Combatants)
	{
		if (IsValid(Stats.Combatant))
		{
			Sorted.Add(Stats);
		}
	}
	Sorted.StableSort([](const FArenaCombatantStats& A, const FArenaCombatantStats& B)
	{
		if (A.Score != B.Score) { return A.Score > B.Score; }
		if (A.Kills != B.Kills) { return A.Kills > B.Kills; }
		if (A.Deaths != B.Deaths) { return A.Deaths < B.Deaths; }
		return A.bIsPlayer && !B.bIsPlayer;
	});
	return Sorted;
}

bool AArenaGameMode::GetStatsFor(const AArenaCharacter* Combatant, FArenaCombatantStats& OutStats) const
{
	if (const FArenaCombatantStats* Stats = FindStats(Combatant))
	{
		OutStats = *Stats;
		return true;
	}
	return false;
}

int32 AArenaGameMode::GetRankOf(const AArenaCharacter* Combatant) const
{
	const TArray<FArenaCombatantStats> Sorted = GetSortedStats();
	for (int32 Index = 0; Index < Sorted.Num(); ++Index)
	{
		if (Sorted[Index].Combatant == Combatant)
		{
			return Index + 1;
		}
	}
	return 0;
}

float AArenaGameMode::GetTimeRemaining() const
{
	if (Settings.TimeLimitMinutes <= 0.f)
	{
		return -1.f;
	}
	if (bMatchOver)
	{
		return FMath::Max(0.f, Settings.TimeLimitMinutes * 60.f - (MatchEndTime - MatchStartTime));
	}
	return GetWorldTimerManager().GetTimerRemaining(TimeLimitTimer);
}

float AArenaGameMode::GetMatchElapsedTime() const
{
	const float End = bMatchOver ? MatchEndTime : GetWorld()->GetTimeSeconds();
	return FMath::Max(0.f, End - MatchStartTime);
}

// ==================================================================== End of match

void AArenaGameMode::HandleTimeLimitReached()
{
	EndMatch(EArenaMatchEndReason::TimeLimit);
}

void AArenaGameMode::EndMatch(EArenaMatchEndReason Reason)
{
	if (bMatchOver)
	{
		return;
	}
	bMatchOver = true;
	EndReason = Reason;
	MatchEndTime = GetWorld()->GetTimeSeconds();
	GetWorldTimerManager().ClearTimer(TimeLimitTimer);

	const TArray<FArenaCombatantStats> Sorted = GetSortedStats();
	Winner = Sorted.Num() > 0 ? Sorted[0].Combatant.Get() : nullptr;

	for (const FArenaCombatantStats& Stats : Combatants)
	{
		if (AArenaCharacter* Character = Stats.Combatant)
		{
			Character->FreezeForMatchEnd();
			if (AArenaAIController* AIController = Cast<AArenaAIController>(Character->GetController()))
			{
				AIController->OnMatchEnded();
			}
		}
	}

	OnMatchEnded.Broadcast(Winner.Get(), Reason);

	// Brief slow-motion beat, then the end screen. Timers run on dilated time, so scale the delay to keep it in real seconds.
	const float Dilation = FMath::Clamp(EndMatchSlowMotion, 0.05f, 1.f);
	UGameplayStatics::SetGlobalTimeDilation(this, Dilation);
	GetWorldTimerManager().SetTimer(EndScreenTimer, this, &AArenaGameMode::ShowEndScreens, FMath::Max(0.01f, EndScreenDelay * Dilation), false);
}

void AArenaGameMode::ShowEndScreens()
{
	UGameplayStatics::SetGlobalTimeDilation(this, 1.f);

	bool bAllShown = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AArenaPlayerController* PlayerController = Cast<AArenaPlayerController>(It->Get()))
		{
			bAllShown = PlayerController->ShowEndOfMatch() && bAllShown;
		}
	}

	// Only pause if there is a screen with buttons to leave from.
	if (bAllShown)
	{
		UGameplayStatics::SetGamePaused(this, true);
	}
}
