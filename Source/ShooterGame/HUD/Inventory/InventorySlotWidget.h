// Source/ShooterGame/HUD/Inventory/InventorySlotWidget.h
#pragma once

#include "CoreMinimal.h"
#include "HUD/Inventory/InventoryBase.h"
#include "HUD/Inventory/InventoryTooltipWidget.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "InventorySlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UOverlay;
class USizeBox;

// ============================================================================
// UInventorySlotWidget
//
// Unified atomic slot widget used by ALL inventory containers
// (equipment, backpack, quick slot, stash).
//
// Sizing and visual config is driven per-container via FInventorySlotSizeConfig
// set on the WBP subclass defaults — never hardcoded here.
//
// ARCHITECTURE RULE: Zero Blueprint event graph logic.
// All state (hover, fill, drag highlight, right-click tooltip) is driven from C++.
// UMG designer is used for layout declaration only.
// ============================================================================
UCLASS()
class SHOOTERGAME_API UInventorySlotWidget : public UInventoryBase
{
    GENERATED_BODY()

public:

    // Pushes SlotSizeConfig values into the bound widgets.
    // Called automatically in NativeConstruct; can be called again at runtime
    // if the config needs to change (e.g. container swap).
    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void ApplySizeConfig(const FInventorySlotSizeConfig& InConfig);

    // Sets the keybind label text and shows/hides KeybindText accordingly.
    // Pass FText::GetEmpty() to hide the label.
    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void SetKeybindLabel(const FText& InText);

    // Syncs all visuals (icon, stack count, keybind, tints) from current ItemInstance.
    // Called automatically after InitializeInventoryEntry and ClearInventoryEntry.
    UFUNCTION(BlueprintCallable, Category = "Inventory Slot")
    void RefreshVisuals();

protected:

    // -------------------------------------------------------------------------
    // UUserWidget interface
    // -------------------------------------------------------------------------
    virtual void NativeConstruct() override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    // -------------------------------------------------------------------------
    // UInventoryBase overrides — all visual responses handled here in C++
    // -------------------------------------------------------------------------
    virtual void BP_OnInventoryEntryInitialized_Implementation() override;
    virtual bool CanAcceptDrop_Implementation(UInventoryDragDropOperation* DragOperation) const override;
    virtual bool HandleDropOperation_Implementation(UInventoryDragDropOperation* DragOperation) override;

    // Called by UInventoryBase when the context menu is closed — we use this
    // to destroy the tooltip at the same time so they stay in sync.
    virtual void BP_OnContextMenuClosed_Implementation();

    // -------------------------------------------------------------------------
    // Bound Widgets — names MUST match exactly in WBP_InventorySlot
    // -------------------------------------------------------------------------
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USizeBox> SizeBox;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> SlotBorder;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> IconImage;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StackCountText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> KeybindText;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> DragHighlightImage;

    // -------------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------------
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Slot|Config")
    FInventorySlotSizeConfig SlotSizeConfig;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Slot|Config")
    TObjectPtr<UTexture2D> EmptySlotTexture;

    // -------------------------------------------------------------------------
    // Tooltip Config
    // -------------------------------------------------------------------------

    // Set this to WBP_InventoryTooltip in the WBP_InventorySlot Class Defaults.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Slot|Tooltip")
    TSubclassOf<UInventoryTooltipWidget> TooltipWidgetClass;

    // Controls how much item detail is shown. Standard is the default.
    // Can be overridden per-container in Class Defaults if needed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory Slot|Tooltip")
    ETooltipDetailLevel TooltipDetailLevel = ETooltipDetailLevel::Standard;

private:

    // Currently active tooltip widget — null when no tooltip is showing
    UPROPERTY()
    TObjectPtr<UInventoryTooltipWidget> ActiveTooltipWidget = nullptr;

    // Creates, populates, and adds the tooltip to the viewport.
    // No-op if TooltipWidgetClass is not set or slot has no valid item.
    void ShowTooltip();

    // Removes the tooltip from viewport and nulls the pointer.
    // Safe to call when ActiveTooltipWidget is already null.
    void HideTooltip();

    // Sets SlotBorder background tint directly — used for hover and reset
    void SetSlotBorderTint(const FLinearColor& InColor);

    // Sets DragHighlightImage tint and visibility
    void SetDragHighlight(bool bVisible, const FLinearColor& InColor);
};