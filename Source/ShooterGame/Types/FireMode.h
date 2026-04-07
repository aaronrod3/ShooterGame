#pragma once

#include "CoreMinimal.h"
#include "FireMode.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	EFM_Safe		UMETA(DisplayName = "Safe"),
	EFM_SemiAuto	UMETA(DisplayName = "Semi Auto"),
	EFM_Burst		UMETA(DisplayName = "Burst"),
	EFM_FullAuto	UMETA(DisplayName = "Full Auto"),

	EFM_Max			UMETA(DisplayName = "DefaultMax")
};


// -----------------------------------------------------------------------
// Team identification — used for friendly fire checks
// All human players share ETeam_Players for now (PvE only)
// Extend with PvP teams when needed
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class ETeam : uint8
{
	ET_NoTeam		UMETA(DisplayName = "No Team"),
	ET_Players		UMETA(DisplayName = "Players"),	// all human players — PvE
	ET_Zombies		UMETA(DisplayName = "Zombies"),	// future enemy team

	ET_MAX			UMETA(Hidden)
};