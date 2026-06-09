// Source/ShooterGame/Types/CombatTypes.h
#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

// -----------------------------------------------------------------------
// ECombatAction
// The single active action the character is performing.
// Replaces the scattered bIsReloading / bIsInteractionActive booleans.
// The animation layer reads this to select the correct montage variant.
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class ECombatAction : uint8
{
    None            UMETA(DisplayName = "None"),
    Firing          UMETA(DisplayName = "Firing"),
    Reloading       UMETA(DisplayName = "Reloading"),
    Inspecting      UMETA(DisplayName = "Inspecting"),
    MagCheck        UMETA(DisplayName = "Mag Check"),
    FireModeSwitch  UMETA(DisplayName = "Fire Mode Switch"),
    Interacting     UMETA(DisplayName = "Interacting"),
    Meleeing        UMETA(DisplayName = "Meleeing"),
    ThrowingGrenade UMETA(DisplayName = "Throwing Grenade"),
    Healing         UMETA(DisplayName = "Healing"),
};

// -----------------------------------------------------------------------
// EReloadType
// Which reload variant to play. Replaces the single PlayReloadMontage call
// with a discriminated value the anim layer can query directly.
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class EReloadType : uint8
{
    None        UMETA(DisplayName = "None"),
    Normal      UMETA(DisplayName = "Normal"),       // mag has rounds remaining
    Empty       UMETA(DisplayName = "Empty"),        // mag fully empty, must chamber
    Quick       UMETA(DisplayName = "Quick"),        // inventory has a partial mag ready
};

// -----------------------------------------------------------------------
// EWeaponGrip
// Controls which left-hand grip pose the character uses.
// Drives grip-blend pose selection in the TP anim instance.
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class EWeaponGrip : uint8
{
    Default         UMETA(DisplayName = "Default"),
    GripVertical    UMETA(DisplayName = "Vertical Foregrip"),
    GripAngled      UMETA(DisplayName = "Angled Foregrip"),
};