// Source/ShooterGame/HUD/Inventory/InventoryItemRowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "HUD/Inventory/InventoryBase.h"
#include "InventoryItemRowWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

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
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void BP_OnInventoryEntryInitialized_Implementation() override;
	virtual void NativePreConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory Item Row")
	void BP_OnItemRowSelectionChanged(bool bInSelected);

	void RefreshItemRowVisuals();
	void RefreshSlotTileState();
	void RefreshItemContent();
	void RefreshSelectionTint();

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_SlotTile = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UImage> Image_ItemIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Keybind = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ItemName = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_StackCount = nullptr;

	// Unoccupied/default slot tile art.
	// Assign in the WBP_InventoryItemRow class defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Item Row|Style", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> UnoccupiedSlotTexture = nullptr;

	// Occupied slot tile art.
	// Assign in the WBP_InventoryItemRow class defaults.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Item Row|Style", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> OccupiedSlotTexture = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item Row", meta = (AllowPrivateAccess = "true"))
	bool bIsSelected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory Item Row", meta = (AllowPrivateAccess = "true"))
	bool bIsHovered = false;
};