// Source/ShooterGame/Items/Ammo/AmmoData.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShooterGame/Types/AmmoType.h"
#include "AmmoData.generated.h"

/**
 * UAmmoData
 * Per-caliber data asset. Assign one to each weapon blueprint to define
 * damage values for that caliber. Designer-editable without recompiling.
 */
UCLASS(BlueprintType)
class SHOOTERGAME_API UAmmoData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Which caliber this data asset represents
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	EAmmoType AmmoType = EAmmoType::EAT_9mm;

	// Base damage applied per hit on a body shot
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Damage")
	float BaseDamage = 20.f;

	/**
	 * Multiplier applied to BaseDamage on a headshot.
	 * e.g. BaseDamage=25, HeadShotMultiplier=2.0 → 50 head damage.
	 * Designed to be expanded later into a TMap<FName, float> HitZoneMultipliers
	 * keyed on bone name for full per-hitbox scaling.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Damage",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float HeadShotMultiplier = 2.f;
};
