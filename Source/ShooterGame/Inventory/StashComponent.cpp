// Source/ShooterGame/Inventory/StashComponent.cpp

#include "Inventory/StashComponent.h"
#include "Inventory/ItemDefinition.h"

UStashComponent::UStashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	SetMaxItems(0); // 0 = unlimited stash capacity
}

void UStashComponent::BeginPlay()
{
	Super::BeginPlay();
}

TArray<FItemInstance> UStashComponent::GetItemsByCategory(EItemCategory Category) const
{
	TArray<FItemInstance> FilteredItems;

	for (const FItemInstance& Item : GetItems())
	{
		if (Item.Definition == nullptr)
		{
			continue;
		}

		if (Item.Definition->ItemCategory == Category)
		{
			FilteredItems.Add(Item);
		}
	}

	return FilteredItems;
}

TArray<FItemInstance> UStashComponent::GetAllStashItems() const
{
	return GetItems();
}