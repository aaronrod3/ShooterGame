// Source/ShooterGame/HUD/Inventory/EquipmentSlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "HUD/Inventory/InventoryBase.h"
#include "HUD/Inventory/EquipmentSlotConfigDataAsset.h"
#include "EquipmentSlotWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentSlotExpandedChanged, UEquipmentSlotWidget*, SlotWidget, bool, bIsExpanded);

UCLASS()
class SHOOTERGAME_API UEquipmentSlotWidget : public UInventoryBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	void ApplySlotConfig(const FEquipmentSlotUIConfig& InSlotConfig);

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	void SetExpanded(bool bInExpanded);

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	void ToggleExpanded();

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	const FEquipmentSlotUIConfig& GetSlotConfig() const { return SlotConfig; }

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	bool IsExpanded() const { return bIsExpanded; }

	UFUNCTION(BlueprintPure, Category = "Equipment Slot")
	bool IsExpandable() const { return SlotConfig.bIsExpandable; }

	UPROPERTY(BlueprintAssignable, Category = "Equipment Slot|Events")
	FOnEquipmentSlotExpandedChanged OnEquipmentSlotExpandedChanged;

protected:
	virtual bool CanAcceptDrop_Implementation(UInventoryDragDropOperation* DragOperation) const override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot")
	void BP_OnSlotConfigApplied();

	UFUNCTION(BlueprintImplementableEvent, Category = "Equipment Slot")
	void BP_OnExpandedChanged(bool bInExpanded);

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot", meta = (AllowPrivateAccess = "true"))
	FEquipmentSlotUIConfig SlotConfig;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment Slot", meta = (AllowPrivateAccess = "true"))
	bool bIsExpanded = false;
};