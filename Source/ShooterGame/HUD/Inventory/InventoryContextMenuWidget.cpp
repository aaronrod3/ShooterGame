// Source/ShooterGame/HUD/Inventory/ItemContextMenuWidget.cpp
#include "ShooterGame/HUD/Inventory/InventoryContextMenuWidget.h"

void UInventoryContextMenuWidget::InitializeContextMenu(const FItemContextMenuData& InMenuData)
{
	MenuData = InMenuData;
	BP_OnContextMenuInitialized(MenuData);
}

void UInventoryContextMenuWidget::SelectContextAction(EItemContextAction SelectedAction)
{
	OnItemContextActionSelected.Broadcast(SelectedAction);
}