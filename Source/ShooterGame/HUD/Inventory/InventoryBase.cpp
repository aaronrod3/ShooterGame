// Source/ShooterGame/HUD/Inventory/InventoryBase.cpp
#include "HUD/Inventory/InventoryBase.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "HUD/Inventory/InventoryDragDropOperation.h"
#include "HUD/Inventory/InventoryContextMenuWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquippedStateComponent.h"
#include "Inventory/StashComponent.h"
#include "Player/Character/ShooterGameCharacter.h"

void UInventoryBase::InitializeInventoryEntry(
	UInventoryComponent* InOwningInventory,
	const FItemInstance& InItemInstance,
	EEquipmentContainerType InContainerType,
	EEquipmentSlot InEquipmentSlot)
{
	OwningInventoryComponent = InOwningInventory;
	ItemInstance = InItemInstance;
	ContainerType = InContainerType;
	EquipmentSlot = InEquipmentSlot;

	OnInventoryWidgetItemChanged.Broadcast(ItemInstance);
	BP_OnInventoryEntryInitialized();
}

void UInventoryBase::ClearInventoryEntry()
{
	ItemInstance = FItemInstance();
	OwningInventoryComponent = nullptr;
	ContainerType = EEquipmentContainerType::None;
	EquipmentSlot = EEquipmentSlot::MAX;

	BP_OnInventoryEntryCleared();
}

bool UInventoryBase::HasValidItem() const
{
	return ItemInstance.IsValid();
}

void UInventoryBase::OpenItemContextMenu()
{
	if (!HasValidItem() || !ContextMenuWidgetClass || ActiveContextMenuWidget)
	{
		return;
	}

	ActiveContextMenuWidget = CreateWidget<UInventoryContextMenuWidget>(this, ContextMenuWidgetClass);
	if (!ActiveContextMenuWidget)
	{
		return;
	}

	ActiveContextMenuWidget->InitializeContextMenu(BuildContextMenuData());
	ActiveContextMenuWidget->OnItemContextActionSelected.AddDynamic(this, &UInventoryBase::HandleContextActionSelected);
	ActiveContextMenuWidget->AddToViewport(100);

	BP_OnContextMenuOpened(ActiveContextMenuWidget);
}

void UInventoryBase::CloseItemContextMenu()
{
	if (!ActiveContextMenuWidget)
	{
		return;
	}

	ActiveContextMenuWidget->OnItemContextActionSelected.RemoveDynamic(this, &UInventoryBase::HandleContextActionSelected);
	ActiveContextMenuWidget->RemoveFromParent();
	ActiveContextMenuWidget = nullptr;

	BP_OnContextMenuClosed();
}

FReply UInventoryBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && HasValidItem())
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UInventoryBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && HasValidItem())
	{
		OpenItemContextMenu();
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UInventoryBase::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!HasValidItem())
	{
		return;
	}

	TSubclassOf<UInventoryDragDropOperation> OperationClass = DragDropOperationClass;
	if (!OperationClass)
	{
		OperationClass = UInventoryDragDropOperation::StaticClass();
	}

	UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(OperationClass));

	if (!DragOperation)
	{
		return;
	}

	DragOperation->DraggedItem = ItemInstance;
	DragOperation->DraggedItemInstanceID = ItemInstance.InstanceID;
	DragOperation->SourceInventory = OwningInventoryComponent;
	DragOperation->SourceContainerType = ContainerType;
	DragOperation->SourceEquipmentSlot = EquipmentSlot;

	OutOperation = DragOperation;
	BP_OnDragStarted(DragOperation);
}

bool UInventoryBase::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	if (UInventoryDragDropOperation* DragOperation = Cast<UInventoryDragDropOperation>(InOperation))
	{
		if (!CanAcceptDrop(DragOperation))
		{
			BP_OnDropRejected(DragOperation);
			return false;
		}

		return HandleDropOperation(DragOperation);
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

FItemContextMenuData UInventoryBase::BuildContextMenuData_Implementation() const
{
	FItemContextMenuData MenuData;
	MenuData.ItemInstance = ItemInstance;
	MenuData.SourceContainerType = ContainerType;
	MenuData.SourceEquipmentSlot = EquipmentSlot;
	MenuData.bAllowInspect = HasValidItem();
	MenuData.bAllowSplitStack = HasValidItem() && ItemInstance.StackCount > 1;
	MenuData.bAllowMergeStack = false;
	MenuData.bAllowDrop = ContainerType != EEquipmentContainerType::Stash;

	switch (ContainerType)
	{
	case EEquipmentContainerType::Stash:
		MenuData.bAllowEquip = true;
		MenuData.bAllowMoveToStash = false;
		MenuData.bAllowMoveToBackpack = false;
		break;

	case EEquipmentContainerType::Rig:
	case EEquipmentContainerType::Belt:
	case EEquipmentContainerType::Mission:
		MenuData.bAllowEquip = false;
		MenuData.bAllowMoveToStash = true;
		MenuData.bAllowMoveToBackpack = true;
		break;

	case EEquipmentContainerType::Backpack:
		MenuData.bAllowEquip = false;
		MenuData.bAllowMoveToStash = true;
		MenuData.bAllowMoveToBackpack = false;
		break;

	case EEquipmentContainerType::None:
	default:
		MenuData.bAllowEquip = false;
		MenuData.bAllowMoveToStash = false;
		MenuData.bAllowMoveToBackpack = false;
		break;
	}

	if (EquipmentSlot != EEquipmentSlot::MAX)
	{
		MenuData.bAllowEquip = false;
		MenuData.bAllowMoveToStash = true;
		MenuData.bAllowMoveToBackpack = true;
	}

	return MenuData;
}

void UInventoryBase::HandleContextActionSelected_Implementation(EItemContextAction SelectedAction)
{
	if (!HasValidItem())
	{
		CloseItemContextMenu();
		return;
	}

	AShooterGameCharacter* OwningCharacter = GetOwningPlayerPawn<AShooterGameCharacter>();
	if (!OwningCharacter)
	{
		CloseItemContextMenu();
		return;
	}

	switch (SelectedAction)
	{
	case EItemContextAction::Equip:
		// Step 5 foundation only:
		// actual slot-picking UX / specific target slot routing will be expanded later.
		break;

	case EItemContextAction::MoveToStash:
		if (OwningInventoryComponent && OwningCharacter->GetStash())
		{
			OwningInventoryComponent->Server_TransferItemTo(ItemInstance.InstanceID, OwningCharacter->GetStash(), FGameplayTag());
		}
		break;

	case EItemContextAction::MoveToBackpack:
		if (OwningInventoryComponent &&
			OwningCharacter->GetEquippedState() &&
			OwningCharacter->GetEquippedState()->GetBackpackInventory())
		{
			OwningInventoryComponent->Server_TransferItemTo(
				ItemInstance.InstanceID,
				OwningCharacter->GetEquippedState()->GetBackpackInventory(),
				FGameplayTag());
		}
		break;

	case EItemContextAction::SplitStack:
		// Placeholder for later stack split implementation.
		break;

	case EItemContextAction::MergeStack:
		// Placeholder for later stack merge implementation.
		break;

	case EItemContextAction::Inspect:
		// Placeholder for later inspect/details view.
		break;

	case EItemContextAction::Drop:
		// Placeholder for later mission-only world drop implementation.
		break;

	case EItemContextAction::None:
	default:
		break;
	}

	CloseItemContextMenu();
}

bool UInventoryBase::CanAcceptDrop_Implementation(UInventoryDragDropOperation* DragOperation) const
{
	return DragOperation != nullptr && DragOperation->HasValidDraggedItem();
}

bool UInventoryBase::HandleDropOperation_Implementation(UInventoryDragDropOperation* DragOperation)
{
	return false;
}