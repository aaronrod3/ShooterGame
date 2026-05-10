// Source/ShooterGame/HUD/Inventory/LootContainerWidget.cpp
#include "HUD/Inventory/LootContainerWidget.h"

#include "HUD/Inventory/SubContainerWidget.h"
#include "Interaction/LootContainerActor.h"
#include "Inventory/InventoryComponent.h"
#include "Player/Character/ShooterGameCharacter.h"

void ULootContainerWidget::InitializeLootWindow(ALootContainerActor* InLootActor, AShooterGameCharacter* InPlayerCharacter)
{
	LootActor = InLootActor;
	OwningCharacter = InPlayerCharacter;

	if (LootContainerPanel && LootActor)
	{
		LootContainerPanel->InitializeSubContainer(
			LootActor->GetLootInventory(),
			EEquipmentContainerType::Mission);
	}

	if (PlayerInventoryPanel && OwningCharacter)
	{
		// Right panel shows the mission runtime inventory — compressed, no paper doll
		PlayerInventoryPanel->InitializeSubContainer(
			OwningCharacter->GetInventory(),
			EEquipmentContainerType::Mission);
	}

	BP_OnLootWindowInitialized(LootActor, InPlayerCharacter);
}

void ULootContainerWidget::CloseLootWindow()
{
	BP_OnLootWindowClosed();
	RemoveFromParent();
}

void ULootContainerWidget::NativeDestruct()
{
	LootActor = nullptr;
	OwningCharacter = nullptr;

	Super::NativeDestruct();
}