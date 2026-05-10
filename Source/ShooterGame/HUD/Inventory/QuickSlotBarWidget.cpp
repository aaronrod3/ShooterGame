// Source/ShooterGame/HUD/Inventory/QuickSlotBarWidget.cpp
#include "HUD/Inventory/QuickSlotBarWidget.h"

#include "Inventory/QuickSlotComponent.h"

void UQuickSlotBarWidget::InitializeQuickSlotBar(UQuickSlotComponent* InQuickSlotComponent)
{
	if (QuickSlotComponent)
	{
		QuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UQuickSlotBarWidget::HandleQuickSlotsChanged);
	}

	QuickSlotComponent = InQuickSlotComponent;

	if (QuickSlotComponent)
	{
		QuickSlotComponent->OnQuickSlotsChanged.AddDynamic(this, &UQuickSlotBarWidget::HandleQuickSlotsChanged);
	}

	RefreshQuickSlotBar();
}

void UQuickSlotBarWidget::RefreshQuickSlotBar()
{
	TArray<FQuickSlotAssignment> Assignments;
	if (QuickSlotComponent)
	{
		Assignments = QuickSlotComponent->GetAllQuickSlotAssignments();
	}

	BP_OnQuickSlotBarRefreshed(Assignments);
}

void UQuickSlotBarWidget::NativeDestruct()
{
	if (QuickSlotComponent)
	{
		QuickSlotComponent->OnQuickSlotsChanged.RemoveDynamic(this, &UQuickSlotBarWidget::HandleQuickSlotsChanged);
	}

	Super::NativeDestruct();
}

void UQuickSlotBarWidget::HandleQuickSlotsChanged()
{
	RefreshQuickSlotBar();
}