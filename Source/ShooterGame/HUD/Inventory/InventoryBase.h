// Source/ShooterGame/HUD/Inventory/InventoryBase.h
#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ShooterGame/Types/ItemContextMenuTypes.h"
#include "InventoryBase.generated.h"

class UDragDropOperation;
class UInventoryComponent;
class UInventoryDragDropOperation;
class UInventoryContextMenuWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryWidgetItemChanged, const FItemInstance&, ItemInstance);

UCLASS()
class SHOOTERGAME_API UInventoryBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory Widget")
	virtual void InitializeInventoryEntry(
		UInventoryComponent* InOwningInventory,
		const FItemInstance& InItemInstance,
		EEquipmentContainerType InContainerType,
		EEquipmentSlot InEquipmentSlot);

	// C++-only convenience overload — not exposed to UHT/Blueprint
	// Used internally when the slot is a generic container entry (not a named body slot)
	void InitializeInventoryEntry(
		UInventoryComponent* InOwningInventory,
		const FItemInstance& InItemInstance,
		EEquipmentContainerType InContainerType)
	{
		InitializeInventoryEntry(InOwningInventory, InItemInstance, InContainerType, EEquipmentSlot::MAX);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory Widget")
	virtual void ClearInventoryEntry();

	UFUNCTION(BlueprintPure, Category = "Inventory Widget")
	bool HasValidItem() const;

	UFUNCTION(BlueprintPure, Category = "Inventory Widget")
	const FItemInstance& GetItemInstance() const { return ItemInstance; }

	UFUNCTION(BlueprintPure, Category = "Inventory Widget")
	FGuid GetItemInstanceID() const { return ItemInstance.InstanceID; }

	UFUNCTION(BlueprintPure, Category = "Inventory Widget")
	UInventoryComponent* GetOwningInventoryComponent() const { return OwningInventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Inventory Widget")
	EEquipmentContainerType GetContainerType() const { return ContainerType; }

	UFUNCTION(BlueprintPure, Category = "Inventory Widget")
	EEquipmentSlot GetEquipmentSlot() const { return EquipmentSlot; }

	UPROPERTY(BlueprintAssignable, Category = "Inventory Widget|Events")
	FOnInventoryWidgetItemChanged OnInventoryWidgetItemChanged;
	
	UFUNCTION(BlueprintCallable, Category = "Inventory Widget|Context Menu")
	void OpenItemContextMenu();

	UFUNCTION(BlueprintCallable, Category = "Inventory Widget|Context Menu")
	void CloseItemContextMenu();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	

	UFUNCTION(BlueprintNativeEvent, Category = "Inventory Widget")
	bool CanAcceptDrop(UInventoryDragDropOperation* DragOperation) const;
	virtual bool CanAcceptDrop_Implementation(UInventoryDragDropOperation* DragOperation) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Inventory Widget")
	bool HandleDropOperation(UInventoryDragDropOperation* DragOperation);
	virtual bool HandleDropOperation_Implementation(UInventoryDragDropOperation* DragOperation);

	UFUNCTION(BlueprintNativeEvent, Category = "Inventory Widget")
	void BP_OnInventoryEntryInitialized();
	virtual void BP_OnInventoryEntryInitialized_Implementation();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Widget")
	void BP_OnInventoryEntryCleared();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Widget")
	void BP_OnDragStarted(UInventoryDragDropOperation* DragOperation);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Widget")
	void BP_OnDropRejected(UInventoryDragDropOperation* DragOperation);
	
	UFUNCTION(BlueprintNativeEvent, Category = "Inventory Widget|Context Menu")
	FItemContextMenuData BuildContextMenuData() const;
	virtual FItemContextMenuData BuildContextMenuData_Implementation() const;

	UFUNCTION(BlueprintNativeEvent, Category = "Inventory Widget|Context Menu")
	void HandleContextActionSelected(EItemContextAction SelectedAction);
	virtual void HandleContextActionSelected_Implementation(EItemContextAction SelectedAction);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Widget|Context Menu")
	void BP_OnContextMenuOpened(UInventoryContextMenuWidget* ContextMenuWidget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Widget|Context Menu")
	void BP_OnContextMenuClosed();
	
	
	

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Widget", meta = (AllowPrivateAccess = "true"))
	FItemInstance ItemInstance;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Widget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> OwningInventoryComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Widget", meta = (AllowPrivateAccess = "true"))
	EEquipmentContainerType ContainerType = EEquipmentContainerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory Widget", meta = (AllowPrivateAccess = "true"))
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::MAX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Widget", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UInventoryDragDropOperation> DragDropOperationClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Widget|Context Menu", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UInventoryContextMenuWidget> ContextMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Widget|Context Menu", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryContextMenuWidget> ActiveContextMenuWidget = nullptr;
};