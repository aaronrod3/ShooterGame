// Source/ShooterGame/Inventory/ItemDefinition.cpp
#include "ItemDefinition.h"

FPrimaryAssetId UItemDefinition::GetPrimaryAssetId() const
{
	// Returns "ItemDefinition:AssetName" — used by the Asset Manager
	// to catalog and async-stream this asset. The type name must match
	// what you register in DefaultGame.ini under [/Script/Engine.AssetManagerSettings]
	// (Step 2 editor checkpoint below covers this).
	return FPrimaryAssetId(TEXT("ItemDefinition"), GetFName());
}