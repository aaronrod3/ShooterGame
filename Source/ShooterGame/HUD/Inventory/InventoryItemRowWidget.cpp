// Source/ShooterGame/HUD/Inventory/InventoryItemRowWidget.cpp
#include "HUD/Inventory/InventoryItemRowWidget.h"

void UInventoryItemRowWidget::SetItemRowSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}

	bIsSelected = bInSelected;
	BP_OnItemRowSelectionChanged(bIsSelected);
}