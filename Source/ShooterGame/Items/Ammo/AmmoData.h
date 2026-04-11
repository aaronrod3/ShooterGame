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
 *
 * Damage model (two zones only):
 *   Body shot  → BaseDamage
 *   Head shot  → BaseDamage * HeadShotMultiplier
 *
 * Head detection is handled by AProjectile::OnHit() checking the hit bone name.
 * All other bones fall through to BaseDamage.
 */
UCLASS(BlueprintType)
class SHOOTERGAME_API UAmmoData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	// Which caliber this data asset represents
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	EAmmoType AmmoType = EAmmoType::EAT_9mm;

	// Base damage applied per hit — used for all non-head body shots
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Damage",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float BaseDamage = 20.f;

	// Multiplier applied to BaseDamage on a confirmed headshot
	// e.g. BaseDamage=55, HeadShotMultiplier=2.0 → 110 head damage
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo|Damage",
		meta = (ClampMin = "1.0", UIMin = "1.0"))
	float HeadShotMultiplier = 2.f;
};
