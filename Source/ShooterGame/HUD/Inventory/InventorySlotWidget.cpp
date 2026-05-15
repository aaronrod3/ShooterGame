// Source/ShooterGame/HUD/Inventory/InventorySlotWidget.cpp
#include "HUD/Inventory/InventorySlotWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HUD/Inventory/InventoryDragDropOperation.h"
#include "HUD/Inventory/InventoryTooltipWidget.h"

// ============================================================================
// NativeConstruct
// ============================================================================

void UInventorySlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ApplySizeConfig(SlotSizeConfig);
    RefreshVisuals();
}

// ============================================================================
// ApplySizeConfig
// ============================================================================

void UInventorySlotWidget::ApplySizeConfig(const FInventorySlotSizeConfig& InConfig)
{
    SlotSizeConfig = InConfig;

    if (SizeBox)
    {
        SizeBox->SetWidthOverride(InConfig.SlotSize);
        SizeBox->SetHeightOverride(InConfig.SlotSize);
    }

    if (SlotBorder)
    {
        SlotBorder->SetPadding(FMargin(InConfig.IconPadding));
    }

    auto ApplyFontSize = [&](UTextBlock* TextBlock)
    {
        if (!TextBlock) return;
        FSlateFontInfo FontInfo = TextBlock->GetFont();
        FontInfo.Size = FMath::RoundToInt(InConfig.LabelFontSize);
        TextBlock->SetFont(FontInfo);
    };

    ApplyFontSize(StackCountText);
    ApplyFontSize(KeybindText);
}

// ============================================================================
// RefreshVisuals
// ============================================================================

void UInventorySlotWidget::RefreshVisuals()
{
    const bool bHasItem = HasValidItem();

    if (IconImage)
    {
        if (bHasItem && ItemInstance.Definition)
        {
            // TODO: replace with ItemInstance.Definition->ThumbnailIcon
            // once icon wiring is confirmed from Phase 1 stub
            IconImage->SetBrushFromTexture(nullptr);
        }
        else if (EmptySlotTexture)
        {
            IconImage->SetBrushFromTexture(EmptySlotTexture, true);
        }
        else
        {
            IconImage->SetBrushFromTexture(nullptr);
        }
    }

    if (StackCountText)
    {
        const int32 Count = bHasItem ? ItemInstance.StackCount : 0;
        if (Count > 1)
        {
            StackCountText->SetText(FText::AsNumber(Count));
            StackCountText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            StackCountText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    SetDragHighlight(false, FLinearColor::Transparent);
    SetSlotBorderTint(FLinearColor::Transparent);
}

// ============================================================================
// SetKeybindLabel
// ============================================================================

void UInventorySlotWidget::SetKeybindLabel(const FText& InText)
{
    if (!KeybindText) return;

    if (InText.IsEmpty())
    {
        KeybindText->SetVisibility(ESlateVisibility::Collapsed);
    }
    else
    {
        KeybindText->SetText(InText);
        KeybindText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

// ============================================================================
// Hover
// ============================================================================

void UInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    SetSlotBorderTint(SlotSizeConfig.HoverTint);
}

void UInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    SetSlotBorderTint(FLinearColor::Transparent);

    // If the mouse leaves while the tooltip is open but the context menu
    // is NOT open, close the tooltip. If the context menu IS open, leave
    // it alive — the user is still interacting with this slot's popup.
    if (ActiveTooltipWidget && !ActiveContextMenuWidget)
    {
        HideTooltip();
    }
}

// ============================================================================
// Right-Click — show tooltip then let base open the context menu
// ============================================================================

FReply UInventorySlotWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // Show the tooltip first so it is visible by the time the context menu opens
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && HasValidItem())
    {
        ShowTooltip();
    }

    // Always call Super — UInventoryBase::NativeOnMouseButtonUp handles
    // OpenItemContextMenu() on right-click and all other button-up logic
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

// ============================================================================
// Context menu closed — tear down tooltip at the same time
// ============================================================================

void UInventorySlotWidget::BP_OnContextMenuClosed_Implementation()
{
    // Do NOT call Super — BP_OnContextMenuClosed is a BlueprintImplementableEvent
    // on UInventoryBase, meaning it has no C++ super body to call.
    HideTooltip();
}

// ============================================================================
// Drag Enter / Leave
// ============================================================================

void UInventorySlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

    UInventoryDragDropOperation* InventoryOp = Cast<UInventoryDragDropOperation>(InOperation);
    if (!InventoryOp) return;

    const bool bCanDrop = CanAcceptDrop(InventoryOp);
    SetDragHighlight(true, bCanDrop ? SlotSizeConfig.ValidDropTint : SlotSizeConfig.InvalidDropTint);
}

void UInventorySlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDragLeave(InDragDropEvent, InOperation);
    SetDragHighlight(false, FLinearColor::Transparent);
}

// ============================================================================
// UInventoryBase overrides
// ============================================================================

void UInventorySlotWidget::BP_OnInventoryEntryInitialized_Implementation()
{
    Super::BP_OnInventoryEntryInitialized_Implementation();
    RefreshVisuals();
}

bool UInventorySlotWidget::CanAcceptDrop_Implementation(UInventoryDragDropOperation* DragOperation) const
{
    if (!DragOperation) return false;

    if (DragOperation->DraggedItemInstanceID == GetItemInstanceID()
        && GetItemInstanceID().IsValid())
    {
        return false;
    }

    return Super::CanAcceptDrop_Implementation(DragOperation);
}

bool UInventorySlotWidget::HandleDropOperation_Implementation(UInventoryDragDropOperation* DragOperation)
{
    SetDragHighlight(false, FLinearColor::Transparent);
    return Super::HandleDropOperation_Implementation(DragOperation);
}

// ============================================================================
// Tooltip lifecycle
// ============================================================================

void UInventorySlotWidget::ShowTooltip()
{
    // Guard — need a class assigned and a valid item
    if (!TooltipWidgetClass || !HasValidItem()) return;

    // If one is already showing (e.g. double right-click), reuse it
    if (ActiveTooltipWidget)
    {
        ActiveTooltipWidget->PopulateFromItem(ItemInstance, TooltipDetailLevel);
        return;
    }

    ActiveTooltipWidget = CreateWidget<UInventoryTooltipWidget>(GetOwningPlayer(), TooltipWidgetClass);
    if (!ActiveTooltipWidget) return;

    ActiveTooltipWidget->PopulateFromItem(ItemInstance, TooltipDetailLevel);

    // Z-order 100 keeps the tooltip above all inventory container panels
    ActiveTooltipWidget->AddToViewport(100);

    // Position anchored just to the right of the slot in viewport space
    const FVector2D SlotScreenPos = GetCachedGeometry().GetAbsolutePosition();
    const FVector2D SlotSize      = GetCachedGeometry().GetAbsoluteSize();
    ActiveTooltipWidget->SetPositionInViewport(
        FVector2D(SlotScreenPos.X + SlotSize.X + 8.f, SlotScreenPos.Y),
        /*bRemoveDPIScale=*/false);
}

void UInventorySlotWidget::HideTooltip()
{
    if (ActiveTooltipWidget)
    {
        ActiveTooltipWidget->RemoveFromParent();
        ActiveTooltipWidget = nullptr;
    }
}

// ============================================================================
// Private Helpers
// ============================================================================

void UInventorySlotWidget::SetSlotBorderTint(const FLinearColor& InColor)
{
    if (SlotBorder)
    {
        SlotBorder->SetBrushColor(InColor);
    }
}

void UInventorySlotWidget::SetDragHighlight(bool bVisible, const FLinearColor& InColor)
{
    if (!DragHighlightImage) return;

    if (bVisible)
    {
        DragHighlightImage->SetColorAndOpacity(InColor);
        DragHighlightImage->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        DragHighlightImage->SetVisibility(ESlateVisibility::Collapsed);
    }
}