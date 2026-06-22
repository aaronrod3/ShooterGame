// Source/ShooterGame/Items/Weapon/WeaponConfig.cpp
#include "Items/Weapon/WeaponConfig.h"

FPrimaryAssetId UWeaponConfig::GetPrimaryAssetId() const
{
	// "WeaponConfig" must match the PrimaryAssetType string registered
	// in DefaultGame.ini under [/Script/Engine.AssetManagerSettings].
	return FPrimaryAssetId(FName(TEXT("WeaponConfig")), GetFName());
}