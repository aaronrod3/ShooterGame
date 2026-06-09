// Source/ShooterGame/Items/Weapon/WeaponConfig.cpp
#include "Items/Weapon/WeaponConfig.h"

FPrimaryAssetId UWeaponConfig::GetPrimaryAssetId() const
{
	// Asset type label used by the Asset Manager when registering
	// config assets.  Match this string in DefaultGame.ini under
	// [/Script/Engine.AssetManagerSettings] PrimaryAssetTypesToScan.
	return FPrimaryAssetId(FName("WeaponConfig"), GetFName());
}