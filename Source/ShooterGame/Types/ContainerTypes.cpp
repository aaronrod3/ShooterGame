// Source/ShooterGame/Types/ContainerTypes.cpp

#include "ShooterGame/Types/ContainerTypes.h"
#include "Inventory/ItemDefinition.h"

float FEquippedContainerState::GetTotalWeight() const
{
	float TotalWeightKg = 0.0f;

	for (const FItemInstance& Item : Items)
	{
		if (Item.Definition == nullptr)
		{
			continue;
		}

		const int32 EffectiveStackCount = FMath::Max(1, Item.StackCount);
		TotalWeightKg += Item.Definition->Weight * EffectiveStackCount;
	}

	return TotalWeightKg;
}

void FEquippedStateSnapshot::ClearAll()
{
	PrimaryWeapon.Clear();
	SecondaryWeapon.Clear();
	SidearmWeapon.Clear();
	HelmetItem.Clear();
	ChestItem.Clear();
	ToolItem.Clear();
	KnifeItem.Clear();

	RigState.Items.Reset();
	BeltState.Items.Reset();
	BackpackState.Items.Reset();

	CurrentCarriedWeight = 0.0f;
	CurrentBurdenScore = 0.0f;
	DeadPlayerBurdenMultiplier = 1.0f;
}

float FEquippedStateSnapshot::CalculateTotalWeight() const
{
	float TotalWeightKg = 0.0f;

	const auto AddItemWeight = [&TotalWeightKg](const FNamedEquippedItem& EquippedItem)
	{
		if (EquippedItem.ItemInstance.Definition == nullptr)
		{
			return;
		}

		const int32 EffectiveStackCount = FMath::Max(1, EquippedItem.ItemInstance.StackCount);
		TotalWeightKg += EquippedItem.ItemInstance.Definition->Weight * EffectiveStackCount;
	};

	AddItemWeight(PrimaryWeapon);
	AddItemWeight(SecondaryWeapon);
	AddItemWeight(SidearmWeapon);
	AddItemWeight(HelmetItem);
	AddItemWeight(ChestItem);
	AddItemWeight(ToolItem);
	AddItemWeight(KnifeItem);

	TotalWeightKg += RigState.GetTotalWeight();
	TotalWeightKg += BeltState.GetTotalWeight();
	TotalWeightKg += BackpackState.GetTotalWeight();

	return TotalWeightKg;
}

void FLoadoutPreset::InitializeDefaultSlots()
{
	EquippedSlots.Reset();

	for (int32 SlotIndex = 0; SlotIndex < static_cast<int32>(EEquipmentSlot::MAX); ++SlotIndex)
	{
		FLoadoutPresetSlot NewSlot;
		NewSlot.EquipmentSlot = static_cast<EEquipmentSlot>(SlotIndex);
		EquippedSlots.Add(NewSlot);
	}
}

void FLoadoutPreset::ClearAll()
{
	for (FLoadoutPresetSlot& Slot : EquippedSlots)
	{
		Slot.Clear();
	}

	RigItemIDs.Reset();
	BeltItemIDs.Reset();
	BackpackItemIDs.Reset();
}