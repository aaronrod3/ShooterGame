// Source/ShooterGame/HUD/Inventory/EquipmentSlotWidget.cpp
#include "HUD/Inventory/EquipmentSlotWidget.h"

#include "HUD/Inventory/InventoryDragDropOperation.h"
#include "Inventory/InventoryComponent.h"

void UEquipmentSlotWidget::ApplySlotConfig(const FEquipmentSlotUIConfig& InSlotConfig)
{
	SlotConfig = InSlotConfig;
	ContainerType = SlotConfig.ContainerType;
	EquipmentSlot = SlotConfig.EquipmentSlot;

	BP_OnSlotConfigApplied();
}

void UEquipmentSlotWidget::SetExpanded(bool bInExpanded)
{
	if (!SlotConfig.bIsExpandable)
	{
		bInExpanded = false;
	}

	if (bIsExpanded == bInExpanded)
	{
		return;
	}

	bIsExpanded = bInExpanded;
	OnEquipmentSlotExpandedChanged.Broadcast(this, bIsExpanded);
	BP_OnExpandedChanged(bIsExpanded);
}

void UEquipmentSlotWidget::ToggleExpanded()
{
	SetExpanded(!bIsExpanded);
}

bool UEquipmentSlotWidget::CanAcceptDrop_Implementation(UInventoryDragDropOperation* DragOperation) const
{
	if (!Super::CanAcceptDrop_Implementation(DragOperation))
	{
		return false;
	}

	if (!SlotConfig.RequiredSlotTag.IsValid())
	{
		return true;
	}

	return UInventoryComponent::IsItemValidForSlot(DragOperation->DraggedItem, SlotConfig.RequiredSlotTag);
}