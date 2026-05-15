// Source/ShooterGame/HUD/Inventory/InventoryTooltipWidget.cpp
#include "HUD/Inventory/InventoryTooltipWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "ShooterGame/Inventory/ItemDefinition.h"

// ============================================================================
// Static constants
// ============================================================================

// Dark panel: nearly black, 85% opacity — matches your dark semi-transparent spec
const FLinearColor UInventoryTooltipWidget::DefaultBorderTint = FLinearColor(0.04f, 0.04f, 0.04f, 0.85f);

// Amber/gold — distinct quest item border highlight
const FLinearColor UInventoryTooltipWidget::QuestBorderTint   = FLinearColor(0.96f, 0.65f, 0.14f, 1.0f);

// ============================================================================
// PopulateFromItem
// ============================================================================

void UInventoryTooltipWidget::PopulateFromItem(const FItemInstance& InItemInstance, ETooltipDetailLevel InDetailLevel)
{
    // Always start with everything optional hidden, then selectively show
    HideAllOptionalRows();

    // Guard — if no valid definition, nothing to display; leave all rows hidden
    if (!InItemInstance.IsValid() || !InItemInstance.Definition)
    {
        if (TooltipRootBorder)
        {
            TooltipRootBorder->SetBrushColor(DefaultBorderTint);
        }
        return;
    }

    const UItemDefinition* Def = InItemInstance.Definition;

    // -------------------------------------------------------------------------
    // Root border tint — quest items get the amber highlight regardless of level
    // -------------------------------------------------------------------------
    if (TooltipRootBorder)
    {
        TooltipRootBorder->SetBrushColor(Def->bIsQuestItem ? QuestBorderTint : DefaultBorderTint);
    }

    // -------------------------------------------------------------------------
    // Always visible — Name, Category, Weight (all three detail levels)
    // -------------------------------------------------------------------------
    if (ItemNameText)
    {
        ItemNameText->SetText(Def->DisplayName);
    }

    if (ItemCategoryText)
    {
        ItemCategoryText->SetText(GetCategoryDisplayText(Def->ItemCategory));
        ItemCategoryText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    if (ItemWeightText)
    {
        const FText WeightLabel = FText::Format(
            NSLOCTEXT("Tooltip", "Weight", "Weight: {0} kg"),
            FText::AsNumber(Def->Weight));
        ItemWeightText->SetText(WeightLabel);
        ItemWeightText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    // -------------------------------------------------------------------------
    // Standard + Full rows
    // -------------------------------------------------------------------------
    if (InDetailLevel == ETooltipDetailLevel::Standard || InDetailLevel == ETooltipDetailLevel::Full)
    {
        // Description
        if (ItemDescriptionText)
        {
            ItemDescriptionText->SetText(Def->Description);
            ItemDescriptionText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }

        // Stack info — only show for stackable items (StackMax > 1)
        if (StackInfoText && Def->StackMax > 1)
        {
            const FText StackLabel = FText::Format(
                NSLOCTEXT("Tooltip", "Stack", "Stack: {0} / {1}"),
                FText::AsNumber(InItemInstance.StackCount),
                FText::AsNumber(Def->StackMax));
            StackInfoText->SetText(StackLabel);
            StackInfoText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }

        // Quest flag
        if (QuestFlagText && Def->bIsQuestItem)
        {
            QuestFlagText->SetText(NSLOCTEXT("Tooltip", "QuestFlag", "\u2691 QUEST ITEM"));
            QuestFlagText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    // -------------------------------------------------------------------------
    // Full-only rows
    // -------------------------------------------------------------------------
    if (InDetailLevel == ETooltipDetailLevel::Full)
    {
        // Vendor buy price — hide if not purchasable
        if (VendorBuyText && Def->VendorBuyPrice > 0)
        {
            const FText BuyLabel = FText::Format(
                NSLOCTEXT("Tooltip", "VendorBuy", "Buy:  \u20BD {0}"),
                FText::AsNumber(Def->VendorBuyPrice));
            VendorBuyText->SetText(BuyLabel);
            VendorBuyText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }

        // Vendor sell price — hide if not sellable (quest items, currency)
        if (VendorSellText && Def->VendorSellPrice > 0)
        {
            const FText SellLabel = FText::Format(
                NSLOCTEXT("Tooltip", "VendorSell", "Sell: \u20BD {0}"),
                FText::AsNumber(Def->VendorSellPrice));
            VendorSellText->SetText(SellLabel);
            VendorSellText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }

        // Durability — hide if item doesn't degrade (RepairMethod == None)
        if (DurabilityText && Def->RepairMethod != EItemRepairMethod::None)
        {
            const FText DuraLabel = FText::Format(
                NSLOCTEXT("Tooltip", "Durability", "Condition: {0} / {1}  ({2})"),
                FText::AsNumber(FMath::RoundToInt(InItemInstance.Durability)),
                FText::AsNumber(FMath::RoundToInt(InItemInstance.MaxCondition)),
                GetRepairMethodText(Def->RepairMethod));
            DurabilityText->SetText(DuraLabel);
            DurabilityText->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }
}

// ============================================================================
// HideAllOptionalRows
// ============================================================================

void UInventoryTooltipWidget::HideAllOptionalRows()
{
    // Name is never hidden — it is always the anchor row
    // Category and Weight are always shown — collapse them only in the
    // guard path above where definition is invalid

    auto Hide = [](UTextBlock* TextBlock)
    {
        if (TextBlock)
        {
            TextBlock->SetVisibility(ESlateVisibility::Collapsed);
        }
    };

    Hide(ItemDescriptionText);
    Hide(StackInfoText);
    Hide(QuestFlagText);
    Hide(VendorBuyText);
    Hide(VendorSellText);
    Hide(DurabilityText);
}

// ============================================================================
// GetCategoryDisplayText
// ============================================================================

FText UInventoryTooltipWidget::GetCategoryDisplayText(EItemCategory InCategory)
{
    // EItemCategory is defined in your existing LoadoutTypes/ItemTypes — 
    // add additional cases here as you expand EItemCategory
    switch (InCategory)
    {
        case EItemCategory::Weapon:      return NSLOCTEXT("Tooltip", "Cat_Weapon",      "Weapon");
        case EItemCategory::Consumable:  return NSLOCTEXT("Tooltip", "Cat_Consumable",  "Consumable");
        case EItemCategory::Equipment:   return NSLOCTEXT("Tooltip", "Cat_Equipment",   "Equipment");
        case EItemCategory::QuestItem:   return NSLOCTEXT("Tooltip", "Cat_Quest",       "Quest Item");
        case EItemCategory::Ammo:        return NSLOCTEXT("Tooltip", "Cat_Ammo",        "Ammunition");
        case EItemCategory::Currency:    return NSLOCTEXT("Tooltip", "Cat_Currency",    "Currency");
    default:                             return NSLOCTEXT("Tooltip", "Cat_Misc",        "Miscellaneous");
    }
}

// ============================================================================
// GetRepairMethodText
// ============================================================================

FText UInventoryTooltipWidget::GetRepairMethodText(EItemRepairMethod InMethod)
{
    switch (InMethod)
    {
    case EItemRepairMethod::Clean:       return NSLOCTEXT("Tooltip", "Repair_Clean",       "Clean");
    case EItemRepairMethod::Sharpen:     return NSLOCTEXT("Tooltip", "Repair_Sharpen",     "Sharpen");
    case EItemRepairMethod::ServiceKit:  return NSLOCTEXT("Tooltip", "Repair_ServiceKit",  "Service Kit");
    case EItemRepairMethod::FieldRepair: return NSLOCTEXT("Tooltip", "Repair_Field",       "Field Repair");
    default:                            return NSLOCTEXT("Tooltip", "Repair_None",         "No Repair");
    }
}