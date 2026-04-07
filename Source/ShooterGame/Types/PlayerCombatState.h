// PlayerCombatState.h
// Defines player lifecycle states, spectator modes, and the downed debuff config struct.
// Used by UDownedComponent, UCombatComponent, AShooterGameCharacter, and AShooterGameSpectatorPawn.

#pragma once

#include "CoreMinimal.h"
#include "PlayerCombatState.generated.h"

// -----------------------------------------------------------------------
// Player combat lifecycle — replicated on AShooterGameCharacter
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class EPlayerCombatState : uint8
{
	EPCS_Alive		UMETA(DisplayName = "Alive"),
	EPCS_Downed		UMETA(DisplayName = "Downed"),
	EPCS_Dead		UMETA(DisplayName = "Dead"),

	EPCS_MAX		UMETA(Hidden)
};

// -----------------------------------------------------------------------
// Spectator camera modes — extensible, add new entries as needed
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class ESpectatorMode : uint8
{
	ESM_FollowPlayer	UMETA(DisplayName = "Follow Player"),

	ESM_MAX				UMETA(Hidden)
};

// -----------------------------------------------------------------------
// All downed state configuration — set per weapon BP or character BP defaults
// -----------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FDownedDebuffConfig
{
	GENERATED_BODY()

	// --- Movement ---

	// MaxWalkSpeed applied when downed (crawl speed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Movement")
	float CrawlSpeed = 100.f;

	// Velocity below this value (cm/s) is treated as idle for bleedout rate purposes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Movement")
	float IdleSpeedThreshold = 10.f;

	// --- Bleedout Rates (seconds of bleedout consumed per real second) ---

	// Idle: not moving, not firing — slowest drain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Bleedout")
	float BleedoutRate_Idle = 0.5f;

	// Moving only
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Bleedout")
	float BleedoutRate_Moving = 1.5f;

	// Firing only
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Bleedout")
	float BleedoutRate_Firing = 1.5f;

	// Moving AND firing — fastest normal drain
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|Bleedout")
	float BleedoutRate_Active = 2.5f;

	// --- Hit Spike ---

	// Bleedout rate during a hit spike — should be higher than BleedoutRate_Active
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|HitSpike")
	float HitSpikeRate = 5.0f;

	// How long (seconds) each hit spike lasts — refreshed on every new hit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|HitSpike")
	float HitSpikeDuration = 3.0f;

	// --- Weapon Debuffs ---

	// Multiplier applied to weapon spread while downed (>1 = more spread)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|WeaponDebuffs")
	float SpreadMultiplier = 2.5f;

	// Multiplier applied to fire rate while downed (<1 = slower fire rate)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Downed|WeaponDebuffs")
	float FireRateMultiplier = 0.6f;
};