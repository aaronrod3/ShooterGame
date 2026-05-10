// Source/ShooterGame/HUD/Inventory/SquadCacheWidget.cpp
#include "HUD/Inventory/SquadCacheWidget.h"

#include "HUD/Inventory/SubContainerWidget.h"
#include "Interaction/SquadCacheActor.h"
#include "Inventory/InventoryComponent.h"
#include "Player/Character/ShooterGameCharacter.h"

void USquadCacheWidget::InitializeSquadCacheWindow(ASquadCacheActor* InCacheActor, AShooterGameCharacter* InPlayerCharacter)
{
	CacheActor = InCacheActor;
	OwningCharacter = InPlayerCharacter;

	if (CacheContainerPanel && CacheActor)
	{
		CacheContainerPanel->InitializeSubContainer(
			CacheActor->GetCacheInventory(),
			EEquipmentContainerType::Mission);
	}

	if (PlayerInventoryPanel && OwningCharacter)
	{
		PlayerInventoryPanel->InitializeSubContainer(
			OwningCharacter->GetInventory(),
			EEquipmentContainerType::Mission);
	}

	BP_OnSquadCacheWindowInitialized(CacheActor, InPlayerCharacter);
}

void USquadCacheWidget::CloseSquadCacheWindow()
{
	BP_OnSquadCacheWindowClosed();
	RemoveFromParent();
}

void USquadCacheWidget::NativeDestruct()
{
	CacheActor = nullptr;
	OwningCharacter = nullptr;

	Super::NativeDestruct();
}