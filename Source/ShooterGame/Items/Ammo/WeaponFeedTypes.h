// Source/ShooterGame/Items/Ammo/WeaponFeedTypes.h
#pragma once

#include "CoreMinimal.h"
#include "ShooterGame/Types/AmmoType.h"
#include "WeaponFeedTypes.generated.h"

/**
 * EWeaponFeedType
 * Defines how a weapon is fed. Controls reload logic, chamber behavior,
 * and whether a detachable magazine is involved.
 */
UENUM(BlueprintType)
enum class EWeaponFeedType : uint8
{
	// Pistols, rifles, SMGs — uses a detachable FMagazine that can be ejected/swapped
	EFT_DetachableMagazine	UMETA(DisplayName = "Detachable Magazine"),

	// Revolvers — fixed cylinder, capacity set per weapon, no detachable mag, no chamber (+1)
	EFT_Cylinder			UMETA(DisplayName = "Cylinder"),

	// RPGs, grenade launchers — single round loaded directly, no magazine object
	EFT_SingleShot			UMETA(DisplayName = "Single Shot"),

	// Tube-fed shotguns — internal fixed capacity, rounds loaded individually (future use)
	EFT_InternalMagazine	UMETA(DisplayName = "Internal Magazine"),

	EFT_MAX					UMETA(DisplayName = "DefaultMAX")
};


/**
 * FMagazine
 * A discrete feed container. Exists both as the magazine currently inserted
 * in a weapon (AWeapon::InsertedMagazine) and as spare magazines held in
 * equipment slots (UInventoryComponent::MagazineSlots).
 *
 * For cylinder weapons:    one FMagazine represents the cylinder itself (Capacity = 6, etc.)
 * For single-shot weapons: one FMagazine represents one loaded round (Capacity = 1)
 * For detachable mags:     one FMagazine per physical magazine object
 */
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FMagazine
{
	GENERATED_BODY()

	// What caliber this magazine holds — must match the weapon's SupportedAmmoType to be inserted
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine")
	EAmmoType AmmoType = EAmmoType::EAT_9mm;

	// Rounds currently loaded in this magazine
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine",
		meta = (ClampMin = "0"))
	int32 CurrentRounds = 0;

	// Maximum rounds this magazine can hold
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Magazine",
		meta = (ClampMin = "1"))
	int32 Capacity = 15;

	// -----------------------------------------------------------------------
	// State Helpers
	// -----------------------------------------------------------------------

	bool IsEmpty() const { return CurrentRounds <= 0; }
	bool IsFull()  const { return CurrentRounds >= Capacity; }
	int32 RoundsRemaining() const { return CurrentRounds; }
	int32 SpaceRemaining()  const { return Capacity - CurrentRounds; }

	// -----------------------------------------------------------------------
	// Mutation Helpers
	// -----------------------------------------------------------------------

	/**
	 * Consumes one round from this magazine.
	 * Returns true if a round was available and consumed.
	 */
	bool ConsumeRound()
	{
		if (CurrentRounds > 0)
		{
			--CurrentRounds;
			return true;
		}
		return false;
	}

	/**
	 * Adds up to Amount rounds without exceeding Capacity.
	 * Returns the number of rounds actually added.
	 */
	int32 AddRounds(int32 Amount)
	{
		int32 Space = SpaceRemaining();
		int32 Added = FMath::Min(Amount, Space);
		CurrentRounds += Added;
		return Added;
	}

	// -----------------------------------------------------------------------
	// Factory Helpers
	// -----------------------------------------------------------------------

	/** Creates a fully loaded magazine for the given caliber and capacity. */
	static FMagazine MakeFull(EAmmoType InAmmoType, int32 InCapacity)
	{
		FMagazine Mag;
		Mag.AmmoType      = InAmmoType;
		Mag.Capacity      = InCapacity;
		Mag.CurrentRounds = InCapacity;
		return Mag;
	}

	/** Creates a partially loaded magazine. */
	static FMagazine MakePartial(EAmmoType InAmmoType, int32 InCapacity, int32 InRounds)
	{
		FMagazine Mag;
		Mag.AmmoType      = InAmmoType;
		Mag.Capacity      = InCapacity;
		Mag.CurrentRounds = FMath::Clamp(InRounds, 0, InCapacity);
		return Mag;
	}

	/** Creates an empty magazine (e.g. ejected dry mag returned to inventory). */
	static FMagazine MakeEmpty(EAmmoType InAmmoType, int32 InCapacity)
	{
		FMagazine Mag;
		Mag.AmmoType      = InAmmoType;
		Mag.Capacity      = InCapacity;
		Mag.CurrentRounds = 0;
		return Mag;
	}

	// -----------------------------------------------------------------------
	// Equality — used by TArray::IndexOfByKey and RemoveMagazine()
	// -----------------------------------------------------------------------

	bool operator==(const FMagazine& Other) const
	{
		return AmmoType      == Other.AmmoType
			&& CurrentRounds == Other.CurrentRounds
			&& Capacity      == Other.Capacity;
	}

	bool operator!=(const FMagazine& Other) const
	{
		return !(*this == Other);
	}
};