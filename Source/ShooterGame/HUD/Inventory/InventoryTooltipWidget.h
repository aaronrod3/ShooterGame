// Source/ShooterGame/HUD/Inventory/InventoryTooltipWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/ContainerTypes.h"   // ETooltipDetailLevel (Step 1)
#include "ShooterGame/Types/ItemTypes.h"        // FItemInstance
#include "InventoryTooltipWidget.generated.h"

class UBorder;
class UTextBlock;

// ============================================================================
// UInventoryTooltipWidget
//
// Display-only widget shown when a player right-clicks an inventory slot
// containing a valid item. Lifecycle (create, populate, destroy) is managed
// entirely in C++ by UInventorySlotWidget — zero Blueprint event graph logic.
//
// Visual rows are shown/hidden per-detail-level by PopulateFromItem().
//
// ARCHITECTURE RULE: This class is NOT a UInventoryBase subclass.
// It owns no item slot and performs no drag/drop logic.
// It is a pure display widget that receives data and renders it.
// ============================================================================
UCLASS()
class SHOOTERGAME_API UInventoryTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Public API — called by UInventorySlotWidget immediately after creation
    // -------------------------------------------------------------------------

    // Populates all visible text fields from InItemInstance.Definition and
    // runtime state on InItemInstance, then shows/hides rows per InDetailLevel.
    // Guard: if InItemInstance.Definition is null, all rows are hidden.
    void PopulateFromItem(const FItemInstance& InItemInstance, ETooltipDetailLevel InDetailLevel);

protected:

    // -------------------------------------------------------------------------
    // Bound Widgets — names MUST match exactly in WBP_InventoryTooltip
    // -------------------------------------------------------------------------

    // Root panel — brush tint swapped to amber border when item is a quest item
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UBorder> TooltipRootBorder;

    // --- Always visible rows (all detail levels) ---

    // Item display name — bold amber
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;

    // Item category (e.g. "Weapon", "Consumable") — muted small
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemCategoryText;

    // Item weight in kg — muted small
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemWeightText;

    // --- Standard + Full rows ---

    // Short description — wrapping white text; hidden in Minimal
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemDescriptionText;

    // Stack info (e.g. "Stack: 3 / 10"); hidden in Minimal and when StackMax == 1
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> StackInfoText;

    // "⚑ QUEST ITEM" label — hidden unless Definition->bIsQuestItem is true
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> QuestFlagText;

    // --- Full only rows ---

    // Vendor buy price (e.g. "Buy: ₽ 4,500"); hidden when VendorBuyPrice == 0
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> VendorBuyText;

    // Vendor sell price (e.g. "Sell: ₽ 2,200"); hidden when VendorSellPrice == 0
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> VendorSellText;

    // Durability info (e.g. "Condition: 74 / 100 — Clean");
    // hidden when RepairMethod == None (quest items, intel docs)
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> DurabilityText;

private:

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    // Hides all optional rows — called first in PopulateFromItem before
    // selectively re-enabling what the current detail level allows.
    void HideAllOptionalRows();

    // Returns a human-readable string for EItemCategory
    static FText GetCategoryDisplayText(EItemCategory InCategory);

    // Returns a human-readable string for EItemRepairMethod
    static FText GetRepairMethodText(EItemRepairMethod InMethod);

    // Tint values for the root border
    static const FLinearColor DefaultBorderTint;   // dark semi-transparent
    static const FLinearColor QuestBorderTint;     // amber/gold outline
};