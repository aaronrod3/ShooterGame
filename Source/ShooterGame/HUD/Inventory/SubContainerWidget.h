// Source/ShooterGame/HUD/Inventory/SubContainerWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "SubContainerWidget.generated.h"

class UInventoryComponent;
class UInventoryItemRowWidget;
class UPanelWidget;

UCLASS()
class SHOOTERGAME_API USubContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Sub Container")
	void InitializeSubContainer(
		UInventoryComponent* InInventoryComponent,
		EEquipmentContainerType InContainerType);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Sub Container")
	void RefreshContainerItems();

	UFUNCTION(BlueprintCallable, Category = "Inventory|Sub Container")
	void ClearContainerItems();

	UFUNCTION(BlueprintPure, Category = "Inventory|Sub Container")
	UInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Sub Container")
	EEquipmentContainerType GetContainerType() const { return ContainerType; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Sub Container")
	UPanelWidget* BP_GetItemHostPanel() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Sub Container")
	void BP_OnSubContainerRefreshed(int32 ItemCount);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Sub Container", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UInventoryItemRowWidget> ItemRowWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Sub Container", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> InventoryComponent = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Sub Container", meta = (AllowPrivateAccess = "true"))
	EEquipmentContainerType ContainerType = EEquipmentContainerType::None;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryItemRowWidget>> SpawnedItemRows;
};