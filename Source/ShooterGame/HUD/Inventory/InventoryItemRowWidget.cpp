// Source/ShooterGame/HUD/Inventory/InventoryItemRowWidget.cpp

#include "HUD/Inventory/InventoryItemRowWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Inventory/ItemDefinition.h"



void UInventoryItemRowWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Show unoccupied slot art by default in editor and at widget creation
	// before any item has been initialized into this row.
	if (Image_SlotTile && UnoccupiedSlotTexture)
	{
		Image_SlotTile->SetBrushFromTexture(UnoccupiedSlotTexture, true);
		Image_SlotTile->SetColorAndOpacity(FLinearColor::White);
	}
}


void UInventoryItemRowWidget::BP_OnInventoryEntryInitialized_Implementation()
{
	Super::BP_OnInventoryEntryInitialized_Implementation();
	
	// Temporary debug — remove after confirming flow
	UE_LOG(LogTemp, Warning, TEXT("UInventoryItemRowWidget: Entry initialized. HasValidItem = %s"),
		HasValidItem() ? TEXT("true") : TEXT("false"));
	
	RefreshItemRowVisuals();
}

void UInventoryItemRowWidget::SetItemRowSelected(bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}

	bIsSelected = bInSelected;
	RefreshSelectionTint();
	BP_OnItemRowSelectionChanged(bInSelected);
}

void UInventoryItemRowWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bIsHovered = true;
	RefreshSelectionTint();
}

void UInventoryItemRowWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bIsHovered = false;
	RefreshSelectionTint();
}

void UInventoryItemRowWidget::RefreshItemRowVisuals()
{
	RefreshSlotTileState();
	RefreshItemContent();
	RefreshSelectionTint();
}

void UInventoryItemRowWidget::RefreshSlotTileState()
{
	if (!Image_SlotTile)
	{
		return;
	}

	const bool bHasValidItem = HasValidItem();
	UTexture2D* SlotTexture = bHasValidItem ? OccupiedSlotTexture : UnoccupiedSlotTexture;

	if (SlotTexture)
	{
		Image_SlotTile->SetBrushFromTexture(SlotTexture, true);
	}

	Image_SlotTile->SetVisibility(ESlateVisibility::Visible);
}

void UInventoryItemRowWidget::RefreshItemContent()
{
	const bool bHasValidItem = HasValidItem();
	const UItemDefinition* ItemDefinition = bHasValidItem ? ItemInstance.Definition.Get() : nullptr;

	if (Image_ItemIcon)
	{
		if (bHasValidItem && ItemDefinition && ItemDefinition->ThumbnailIcon)
		{
			Image_ItemIcon->SetBrushFromTexture(ItemDefinition->ThumbnailIcon, true);
			Image_ItemIcon->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Image_ItemIcon->SetBrush(FSlateBrush());
			Image_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_ItemName)
	{
		if (bHasValidItem && ItemDefinition)
		{
			Text_ItemName->SetText(ItemDefinition->DisplayName);
			Text_ItemName->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Text_ItemName->SetText(FText::GetEmpty());
			Text_ItemName->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_StackCount)
	{
		const bool bShowStackCount =
			bHasValidItem &&
			ItemDefinition &&
			ItemDefinition->StackMax > 1 &&
			ItemInstance.StackCount > 1;

		if (bShowStackCount)
		{
			Text_StackCount->SetText(FText::AsNumber(ItemInstance.StackCount));
			Text_StackCount->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			Text_StackCount->SetText(FText::GetEmpty());
			Text_StackCount->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_Keybind)
	{
		// Keybind text is intentionally deferred to a later phase.
		Text_Keybind->SetText(FText::GetEmpty());
		Text_Keybind->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UInventoryItemRowWidget::RefreshSelectionTint()
{
	if (!Image_SlotTile)
	{
		return;
	}

	// Base = neutral
	FLinearColor TileTint = FLinearColor::White;

	// Hover slightly brightens the tile if not selected.
	if (bIsHovered && !bIsSelected)
	{
		TileTint = FLinearColor(1.08f, 1.08f, 1.08f, 1.0f);
	}

	// Selected state overrides hover with a restrained amber tint.
	if (bIsSelected)
	{
		TileTint = FLinearColor(0.82f, 0.70f, 0.48f, 1.0f);
	}

	Image_SlotTile->SetColorAndOpacity(TileTint);
}