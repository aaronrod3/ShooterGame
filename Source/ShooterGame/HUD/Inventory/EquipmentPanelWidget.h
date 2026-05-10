// Source/ShooterGame/HUD/Inventory/EquipmentPanelWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUD/Inventory/EquipmentSlotConfigDataAsset.h"
#include "EquipmentPanelWidget.generated.h"

class UEquipmentSlotWidget;
class UEquippedStateComponent;
class UPanelWidget;
class USubContainerWidget;

UCLASS()
class SHOOTERGAME_API UEquipmentPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment Panel")
	void InitializeEquipmentPanel(UEquippedStateComponent* InEquippedStateComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Equipment Panel")
	void RefreshEquipmentPanel();

	UFUNCTION(BlueprintPure, Category = "Inventory|Equipment Panel")
	UEquippedStateComponent* GetEquippedStateComponent() const { return EquippedStateComponent; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleEquippedStateChanged();

	// One per EEquipmentSlot entry in your enum:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_Primary;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_Secondary;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_Pistol;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_Tool;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_Consumable1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_Consumable2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Inventory|Equipment Panel")
	TObjectPtr<UEquipmentSlotWidget> Slot_SpecialItem;
	
	

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Equipment Panel")
	void BP_OnEquipmentPanelRefreshed(int32 SlotCount);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Equipment Panel")
	USubContainerWidget* BP_CreateSubContainerWidget(EEquipmentContainerType InContainerType);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Equipment Panel", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentSlotConfigDataAsset> SlotConfigDataAsset = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Equipment Panel", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquippedStateComponent> EquippedStateComponent = nullptr;
	
};