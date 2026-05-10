// Source/ShooterGame/HUD/Inventory/SubContainerWidget.cpp
#include "HUD/Inventory/SubContainerWidget.h"

#include "Components/PanelWidget.h"
#include "HUD/Inventory/InventoryItemRowWidget.h"
#include "Inventory/InventoryComponent.h"

void USubContainerWidget::InitializeSubContainer(
	UInventoryComponent* InInventoryComponent,
	EEquipmentContainerType InContainerType)
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &USubContainerWidget::HandleInventoryChanged);
	}

	InventoryComponent = InInventoryComponent;
	ContainerType = InContainerType;

	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &USubContainerWidget::HandleInventoryChanged);
	}

	RefreshContainerItems();
}

void USubContainerWidget::RefreshContainerItems()
{
	ClearContainerItems();

	UPanelWidget* HostPanel = BP_GetItemHostPanel();
	if (!HostPanel || !InventoryComponent || !ItemRowWidgetClass)
	{
		BP_OnSubContainerRefreshed(0);
		return;
	}

	const TArray<FItemInstance>& Items = InventoryComponent->GetItems();
	for (const FItemInstance& Item : Items)
	{
		UInventoryItemRowWidget* RowWidget = CreateWidget<UInventoryItemRowWidget>(this, ItemRowWidgetClass);
		if (!RowWidget)
		{
			continue;
		}

		RowWidget->InitializeInventoryEntry(InventoryComponent, Item, ContainerType);
		HostPanel->AddChild(RowWidget);
		SpawnedItemRows.Add(RowWidget);
	}

	BP_OnSubContainerRefreshed(SpawnedItemRows.Num());
}

void USubContainerWidget::ClearContainerItems()
{
	if (UPanelWidget* HostPanel = BP_GetItemHostPanel())
	{
		HostPanel->ClearChildren();
	}

	SpawnedItemRows.Reset();
}

void USubContainerWidget::NativeDestruct()
{
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &USubContainerWidget::HandleInventoryChanged);
	}

	Super::NativeDestruct();
}

void USubContainerWidget::HandleInventoryChanged()
{
	RefreshContainerItems();
}