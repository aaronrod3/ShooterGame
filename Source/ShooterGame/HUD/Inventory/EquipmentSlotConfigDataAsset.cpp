// Source/ShooterGame/HUD/Inventory/EquipmentSlotConfigDataAsset.cpp
#include "HUD/Inventory/EquipmentSlotConfigDataAsset.h"

bool UEquipmentSlotConfigDataAsset::GetSlotConfig(EEquipmentSlot InEquipmentSlot, FEquipmentSlotUIConfig& OutConfig) const
{
	for (const FEquipmentSlotUIConfig& Config : SlotConfigs)
	{
		if (Config.EquipmentSlot == InEquipmentSlot)
		{
			OutConfig = Config;
			return true;
		}
	}

	return false;
}

TArray<FEquipmentSlotUIConfig> UEquipmentSlotConfigDataAsset::GetSortedSlotConfigs() const
{
	TArray<FEquipmentSlotUIConfig> SortedConfigs = SlotConfigs;
	SortedConfigs.Sort([](const FEquipmentSlotUIConfig& A, const FEquipmentSlotUIConfig& B)
	{
		return A.SortOrder < B.SortOrder;
	});

	return SortedConfigs;
}