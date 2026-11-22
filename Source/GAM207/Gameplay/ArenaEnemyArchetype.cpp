#include "Gameplay/ArenaEnemyArchetype.h"

namespace
{
	FArenaWeaponStats MakeWeapon(const FText& Name, float Damage, float FireInterval, int32 Projectiles, float Spread, float Speed, float Loudness = 1.f, float NoiseRange = 4000.f)
	{
		FArenaWeaponStats Weapon;
		Weapon.WeaponName = Name;
		Weapon.Damage = Damage;
		Weapon.FireInterval = FireInterval;
		Weapon.ProjectilesPerShot = Projectiles;
		Weapon.SpreadDegrees = Spread;
		Weapon.ProjectileSpeed = Speed;
		Weapon.NoiseLoudness = Loudness;
		Weapon.NoiseRange = NoiseRange;
		return Weapon;
	}

	UArenaEnemyArchetype* MakeArchetype(UObject* Outer, const FText& Name, EArenaPersonality Personality, const FLinearColor& Color, float Scale,
		float Health, float Speed, float PreferredRange, float FleeFraction, float Memory, float Reaction, const FArenaWeaponStats& Weapon)
	{
		UArenaEnemyArchetype* Archetype = NewObject<UArenaEnemyArchetype>(Outer);
		Archetype->DisplayName = Name;
		Archetype->Personality = Personality;
		Archetype->BodyColor = Color;
		Archetype->BodyScale = Scale;
		Archetype->MaxHealth = Health;
		Archetype->WalkSpeed = Speed;
		Archetype->PreferredRange = PreferredRange;
		Archetype->FleeHealthFraction = FleeFraction;
		Archetype->MemoryDuration = Memory;
		Archetype->ReactionTime = Reaction;
		Archetype->Weapon = Weapon;
		return Archetype;
	}
}

void UArenaEnemyArchetype::CreateBuiltInArchetypes(UObject* Outer, TArray<TObjectPtr<UArenaEnemyArchetype>>& OutArchetypes)
{
	// Columns: name, personality, body color, scale, health, walk speed, preferred range, flee fraction, memory (s), reaction (s), weapon.

	// Rusher: fast, fragile, shotgun. Charges in and never backs off.
	OutArchetypes.Add(MakeArchetype(Outer, NSLOCTEXT("Arena", "Rusher", "Rusher"), EArenaPersonality::Aggressive,
		FLinearColor(0.8f, 0.2f, 0.05f), 1.0f, 80.f, 620.f, 450.f, 0.f, 15.f, 0.25f,
		MakeWeapon(NSLOCTEXT("Arena", "Shotgun", "Shotgun"), 7.f, 0.9f, 6, 7.f, 3500.f, 1.2f, 4500.f)));

	// Soldier: the baseline. Rifle, holds mid range.
	OutArchetypes.Add(MakeArchetype(Outer, NSLOCTEXT("Arena", "Soldier", "Soldier"), EArenaPersonality::Defensive,
		FLinearColor(0.15f, 0.3f, 0.6f), 1.0f, 100.f, 450.f, 1100.f, 0.f, 10.f, 0.35f,
		MakeWeapon(NSLOCTEXT("Arena", "Rifle", "Rifle"), 9.f, 0.3f, 1, 3.f, 4500.f)));

	// Marksman: slow, hard-hitting, accurate shots from far away. Its gun is loud.
	OutArchetypes.Add(MakeArchetype(Outer, NSLOCTEXT("Arena", "Marksman", "Marksman"), EArenaPersonality::Defensive,
		FLinearColor(0.1f, 0.45f, 0.15f), 1.0f, 70.f, 400.f, 2000.f, 0.f, 12.f, 0.6f,
		MakeWeapon(NSLOCTEXT("Arena", "Sniper", "Sniper"), 34.f, 1.8f, 1, 0.4f, 9000.f, 1.5f, 6000.f)));

	// Brute: big health pool, slow, pushes forward.
	OutArchetypes.Add(MakeArchetype(Outer, NSLOCTEXT("Arena", "Brute", "Brute"), EArenaPersonality::Aggressive,
		FLinearColor(0.3f, 0.08f, 0.35f), 1.25f, 220.f, 320.f, 700.f, 0.f, 15.f, 0.45f,
		MakeWeapon(NSLOCTEXT("Arena", "HeavyRifle", "Heavy Rifle"), 12.f, 0.45f, 1, 5.f, 3800.f, 1.3f, 5000.f)));

	// Scavenger: cowardly and quick. Keeps its distance and runs for health when hurt.
	OutArchetypes.Add(MakeArchetype(Outer, NSLOCTEXT("Arena", "Scavenger", "Scavenger"), EArenaPersonality::Cowardly,
		FLinearColor(0.75f, 0.65f, 0.1f), 0.9f, 60.f, 640.f, 1400.f, 0.5f, 6.f, 0.4f,
		MakeWeapon(NSLOCTEXT("Arena", "Pistol", "Pistol"), 11.f, 0.6f, 1, 2.5f, 4200.f, 0.8f, 3000.f)));
}
