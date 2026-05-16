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
	// Gate 1: base null/validity guard — always call Super first per contract.
	if (!Super::CanAcceptDrop_Implementation(DragOperation))
	{
		return false;
	}

	// Gate 2: slot container compatibility — checks item's AcceptedSlotTags
	// against this slot's RequiredSlotTag (pre-existing Phase 1 logic).
	if (SlotConfig.RequiredSlotTag.IsValid())
	{
		if (!UInventoryComponent::IsItemValidForSlot(DragOperation->DraggedItem, SlotConfig.RequiredSlotTag))
		{
			return false;
		}
	}

	// Gate 3 (Phase 3): item type tag gating — checks the dragged item's
	// canonical type tag against this slot's AllowedItemTypeTags.
	// Empty AllowedItemTypeTags = permissive (slot accepts any item type).
	if (!SlotConfig.AllowedItemTypeTags.IsEmpty())
	{
		if (!SlotConfig.AllowedItemTypeTags.HasTag(DragOperation->ItemTypeTag))
		{
			return false;
		}
	}

	return true;
}