// Source/ShooterGame/Types/ItemTypes.cpp

#include "ItemTypes.h"
#include "ShooterGame/Inventory/ItemDefinition.h"
#include "ShooterGame/Types/LoadoutTypes.h"

bool FItemInstance::IsWeapon() const
{
	if (!IsValid()) return false;
	return Definition->WeaponCategory != EWeaponCategory::None;
}