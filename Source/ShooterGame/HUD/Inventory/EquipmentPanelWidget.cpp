// Source/ShooterGame/HUD/Inventory/EquipmentPanelWidget.cpp
#include "HUD/Inventory/EquipmentPanelWidget.h"

#include "Components/PanelWidget.h"
#include "HUD/Inventory/EquipmentSlotWidget.h"
#include "HUD/Inventory/SubContainerWidget.h"
#include "Inventory/EquippedStateComponent.h"
#include "Inventory/InventoryComponent.h"

void UEquipmentPanelWidget::InitializeEquipmentPanel(UEquippedStateComponent* InEquippedStateComponent)
{
	if (EquippedStateComponent)
	{
		EquippedStateComponent->OnEquippedStateChanged.RemoveDynamic(this, &UEquipmentPanelWidget::HandleEquippedStateChanged);
	}

	EquippedStateComponent = InEquippedStateComponent;

	if (EquippedStateComponent)
	{
		EquippedStateComponent->OnEquippedStateChanged.AddDynamic(this, &UEquipmentPanelWidget::HandleEquippedStateChanged);
	}

	RefreshEquipmentPanel();
}

void UEquipmentPanelWidget::RefreshEquipmentPanel()
{
	if (!EquippedStateComponent)
	{
		BP_OnEquipmentPanelRefreshed(0);
		return;
	}

	// Helper lambda to initialize one named slot widget
	auto InitSlot = [this](UEquipmentSlotWidget* SlotWidget, EEquipmentSlot InEquipmentSlot, EEquipmentContainerType InContainerType)
	{
		if (!SlotWidget || !SlotConfigDataAsset)
		{
			return;
		}

		FEquipmentSlotUIConfig Config;
		if (SlotConfigDataAsset->GetSlotConfig(InEquipmentSlot, Config))
		{
			SlotWidget->ApplySlotConfig(Config);
		}

		if (const FItemInstance* EquippedItem = EquippedStateComponent->GetEquippedItem(InEquipmentSlot))
		{
			if (EquippedItem->IsValid())
			{
				SlotWidget->InitializeInventoryEntry(nullptr, *EquippedItem, InContainerType, InEquipmentSlot);
				return;
			}
		}

		SlotWidget->ClearInventoryEntry();
	};
	
	

	InitSlot(Slot_Primary,     EEquipmentSlot::Primary,     EEquipmentContainerType::None);
	InitSlot(Slot_Secondary,   EEquipmentSlot::Secondary,   EEquipmentContainerType::None);
	InitSlot(Slot_Pistol,      EEquipmentSlot::Pistol,      EEquipmentContainerType::None);
	InitSlot(Slot_Tool,        EEquipmentSlot::Tool,        EEquipmentContainerType::None);
	InitSlot(Slot_Consumable1, EEquipmentSlot::Consumable1, EEquipmentContainerType::None);
	InitSlot(Slot_Consumable2, EEquipmentSlot::Consumable2, EEquipmentContainerType::None);
	InitSlot(Slot_SpecialItem, EEquipmentSlot::SpecialItem,  EEquipmentContainerType::None);

	BP_OnEquipmentPanelRefreshed(7);
}

void UEquipmentPanelWidget::NativeDestruct()
{
	if (EquippedStateComponent)
	{
		EquippedStateComponent->OnEquippedStateChanged.RemoveDynamic(this, &UEquipmentPanelWidget::HandleEquippedStateChanged);
	}

	Super::NativeDestruct();
}

void UEquipmentPanelWidget::HandleEquippedStateChanged()
{
	RefreshEquipmentPanel();
}