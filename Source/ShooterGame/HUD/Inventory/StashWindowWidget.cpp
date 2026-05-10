// Source/ShooterGame/HUD/Inventory/StashWindowWidget.cpp
#include "HUD/Inventory/StashWindowWidget.h"

#include "Components/PanelWidget.h"
#include "HUD/Inventory/InventoryItemRowWidget.h"
#include "Inventory/StashComponent.h"

void UStashWindowWidget::InitializeStashWindow(UStashComponent* InStashComponent)
{
	if (StashComponent)
	{
		StashComponent->OnInventoryChanged.RemoveDynamic(this, &UStashWindowWidget::HandleStashChanged);
	}

	StashComponent = InStashComponent;

	if (StashComponent)
	{
		StashComponent->OnInventoryChanged.AddDynamic(this, &UStashWindowWidget::HandleStashChanged);
	}

	RefreshStashItems();
}

void UStashWindowWidget::SetActiveCategory(EItemCategory InCategory)
{
	if (ActiveCategory == InCategory)
	{
		return;
	}

	ActiveCategory = InCategory;
	BP_OnActiveCategoryChanged(ActiveCategory);
	RefreshStashItems();
}

void UStashWindowWidget::RefreshStashItems()
{
	if (UPanelWidget* HostPanel = BP_GetStashItemHostPanel())
	{
		HostPanel->ClearChildren();
	}

	SpawnedStashRows.Reset();

	if (!StashComponent || !ItemRowWidgetClass)
	{
		BP_OnStashRefreshed(0);
		return;
	}

	UPanelWidget* HostPanel = BP_GetStashItemHostPanel();
	if (!HostPanel)
	{
		BP_OnStashRefreshed(0);
		return;
	}

	const TArray<FItemInstance> FilteredItems = StashComponent->GetItemsByCategory(ActiveCategory);
	for (const FItemInstance& Item : FilteredItems)
	{
		UInventoryItemRowWidget* RowWidget = CreateWidget<UInventoryItemRowWidget>(this, ItemRowWidgetClass);
		if (!RowWidget)
		{
			continue;
		}

		RowWidget->InitializeInventoryEntry(StashComponent, Item, EEquipmentContainerType::Stash);
		HostPanel->AddChild(RowWidget);
		SpawnedStashRows.Add(RowWidget);
	}

	BP_OnStashRefreshed(SpawnedStashRows.Num());
}

void UStashWindowWidget::NativeDestruct()
{
	if (StashComponent)
	{
		StashComponent->OnInventoryChanged.RemoveDynamic(this, &UStashWindowWidget::HandleStashChanged);
	}

	Super::NativeDestruct();
}

void UStashWindowWidget::HandleStashChanged()
{
	RefreshStashItems();
}