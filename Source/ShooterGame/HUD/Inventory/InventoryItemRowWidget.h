// Source/ShooterGame/HUD/Inventory/InventoryItemRowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "HUD/Inventory/InventoryBase.h"
#include "InventoryItemRowWidget.generated.h"

UCLASS()
class SHOOTERGAME_API UInventoryItemRowWidget : public UInventoryBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory Item Row")
	void SetItemRowSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "Inventory Item Row")
	bool IsItemRowSelected() const { return bIsSelected; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Item Row")
	void BP_OnItemRowSelectionChanged(bool bInSelected);

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item Row", meta = (AllowPrivateAccess = "true"))
	bool bIsSelected = false;
};