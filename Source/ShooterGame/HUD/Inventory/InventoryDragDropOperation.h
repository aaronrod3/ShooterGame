// Source/ShooterGame/HUD/Inventory/InventoryDragDropOperation.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "InventoryDragDropOperation.generated.h"

class UInventoryComponent;

UCLASS()
class SHOOTERGAME_API UInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag Drop")
	FItemInstance DraggedItem;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag Drop")
	FGuid DraggedItemInstanceID;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag Drop")
	TObjectPtr<UInventoryComponent> SourceInventory = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag Drop")
	EEquipmentContainerType SourceContainerType = EEquipmentContainerType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Drag Drop")
	EEquipmentSlot SourceEquipmentSlot = EEquipmentSlot::MAX;

	UFUNCTION(BlueprintCallable, Category = "Inventory Drag Drop")
	bool HasValidDraggedItem() const
	{
		return DraggedItem.IsValid() && DraggedItemInstanceID.IsValid();
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory Drag Drop")
	bool IsFromEquipmentSlot() const
	{
		return SourceEquipmentSlot != EEquipmentSlot::MAX;
	}
};