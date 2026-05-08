// Source/ShooterGame/Inventory/EquippedStateComponent.cpp

#include "Inventory/EquippedStateComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Net/UnrealNetwork.h"

UEquippedStateComponent::UEquippedStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	RigInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("RigInventory"));
	BeltInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("BeltInventory"));
	BackpackInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("BackpackInventory"));
}

void UEquippedStateComponent::BeginPlay()
{
	Super::BeginPlay();
	RecalculateWeight();
}

void UEquippedStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UEquippedStateComponent, PrimaryWeapon);
	DOREPLIFETIME(UEquippedStateComponent, SecondaryWeapon);
	DOREPLIFETIME(UEquippedStateComponent, SidearmWeapon);
	DOREPLIFETIME(UEquippedStateComponent, HelmetItem);
	DOREPLIFETIME(UEquippedStateComponent, ChestItem);
	DOREPLIFETIME(UEquippedStateComponent, ToolItem);
	DOREPLIFETIME(UEquippedStateComponent, KnifeItem);
	DOREPLIFETIME(UEquippedStateComponent, CurrentCarriedWeight);
	DOREPLIFETIME(UEquippedStateComponent, CurrentBurdenScore);
}

bool UEquippedStateComponent::Server_EquipItem(const FGuid& InstanceID, EEquipmentSlot Slot, UInventoryComponent* SourceContainer)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!IsValidEquipmentSlot(Slot) || !IsSingleItemSlot(Slot) || SourceContainer == nullptr)
	{
		return false;
	}

	FItemInstance* TargetSlot = GetEquippedItemMutable(Slot);
	if (TargetSlot == nullptr || TargetSlot->IsValid())
	{
		return false;
	}

	FItemInstance RemovedItem;
	if (!SourceContainer->Server_RemoveItem(InstanceID, RemovedItem))
	{
		return false;
	}

	*TargetSlot = RemovedItem;
	RecalculateWeight();
	BroadcastEquippedStateChanged();
	return true;
}

bool UEquippedStateComponent::Server_UnequipItem(EEquipmentSlot Slot, UInventoryComponent* TargetContainer)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!IsValidEquipmentSlot(Slot) || !IsSingleItemSlot(Slot) || TargetContainer == nullptr)
	{
		return false;
	}

	FItemInstance* EquippedItem = GetEquippedItemMutable(Slot);
	if (EquippedItem == nullptr || !EquippedItem->IsValid())
	{
		return false;
	}

	FItemInstance ItemToMove = *EquippedItem;
	FGameplayTag EmptySlotTag;

	if (!TargetContainer->Server_AddItem(ItemToMove, EmptySlotTag))
	{
		return false;
	}

	*EquippedItem = FItemInstance();
	RecalculateWeight();
	BroadcastEquippedStateChanged();
	return true;
}

bool UEquippedStateComponent::Server_LootDeadPlayer(UEquippedStateComponent* DeadPlayerEquipment)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || DeadPlayerEquipment == nullptr || DeadPlayerEquipment == this)
	{
		return false;
	}

	if (BackpackInventory == nullptr)
	{
		return false;
	}

	TArray<FItemInstance*> DeadEquippedItems =
	{
		&DeadPlayerEquipment->PrimaryWeapon,
		&DeadPlayerEquipment->SecondaryWeapon,
		&DeadPlayerEquipment->SidearmWeapon,
		&DeadPlayerEquipment->HelmetItem,
		&DeadPlayerEquipment->ChestItem,
		&DeadPlayerEquipment->ToolItem,
		&DeadPlayerEquipment->KnifeItem
	};

	bool bMovedAnyItem = false;
	FGameplayTag EmptySlotTag;

	for (FItemInstance* DeadItem : DeadEquippedItems)
	{
		if (DeadItem == nullptr || !DeadItem->IsValid())
		{
			continue;
		}

		FItemInstance LootedItem = *DeadItem;
		if (BackpackInventory->Server_AddItem(LootedItem, EmptySlotTag))
		{
			*DeadItem = FItemInstance();
			bMovedAnyItem = true;
		}
	}

	auto MoveContainerItems = [this, &bMovedAnyItem, &EmptySlotTag](UInventoryComponent* SourceInventory)
	{
		if (SourceInventory == nullptr || BackpackInventory == nullptr)
		{
			return;
		}

		TArray<FItemInstance> ItemsToMove = SourceInventory->GetItems();
		for (const FItemInstance& SourceItem : ItemsToMove)
		{
			FItemInstance RemovedItem;
			if (!SourceInventory->Server_RemoveItem(SourceItem.InstanceID, RemovedItem))
			{
				continue;
			}

			if (BackpackInventory->Server_AddItem(RemovedItem, EmptySlotTag))
			{
				bMovedAnyItem = true;
			}
			else
			{
				SourceInventory->Server_AddItem(RemovedItem, EmptySlotTag);
			}
		}
	};

	MoveContainerItems(DeadPlayerEquipment->RigInventory);
	MoveContainerItems(DeadPlayerEquipment->BeltInventory);
	MoveContainerItems(DeadPlayerEquipment->BackpackInventory);

	if (bMovedAnyItem)
	{
		CurrentBurdenScore *= DeadPlayerBurdenMultiplier;
		RecalculateWeight();
		DeadPlayerEquipment->RecalculateWeight();
		BroadcastEquippedStateChanged();
		DeadPlayerEquipment->BroadcastEquippedStateChanged();
	}

	return bMovedAnyItem;
}

void UEquippedStateComponent::RecalculateWeight()
{
	float TotalWeight = 0.0f;

	TotalWeight += GetItemWeight(PrimaryWeapon);
	TotalWeight += GetItemWeight(SecondaryWeapon);
	TotalWeight += GetItemWeight(SidearmWeapon);
	TotalWeight += GetItemWeight(HelmetItem);
	TotalWeight += GetItemWeight(ChestItem);
	TotalWeight += GetItemWeight(ToolItem);
	TotalWeight += GetItemWeight(KnifeItem);

	if (RigInventory)
	{
		for (const FItemInstance& Item : RigInventory->GetItems())
		{
			TotalWeight += GetItemWeight(Item);
		}
	}

	if (BeltInventory)
	{
		for (const FItemInstance& Item : BeltInventory->GetItems())
		{
			TotalWeight += GetItemWeight(Item);
		}
	}

	if (BackpackInventory)
	{
		for (const FItemInstance& Item : BackpackInventory->GetItems())
		{
			TotalWeight += GetItemWeight(Item);
		}
	}

	CurrentCarriedWeight = TotalWeight;
	CurrentBurdenScore = TotalWeight;
}

const FItemInstance* UEquippedStateComponent::GetEquippedItem(EEquipmentSlot Slot) const
{
	return GetEquippedItemInternal(Slot);
}

bool UEquippedStateComponent::HasEquippedItem(EEquipmentSlot Slot) const
{
	const FItemInstance* Item = GetEquippedItemInternal(Slot);
	return Item != nullptr && Item->IsValid();
}

FItemInstance* UEquippedStateComponent::GetEquippedItemMutable(EEquipmentSlot Slot)
{
	switch (Slot)
	{
	case EEquipmentSlot::Primary:
		return &PrimaryWeapon;
	case EEquipmentSlot::Secondary:
		return &SecondaryWeapon;
	case EEquipmentSlot::Pistol:
		return &SidearmWeapon;
	case EEquipmentSlot::Tool:
		return &ToolItem;
	case EEquipmentSlot::Knife:
		return &KnifeItem;
	default:
		return nullptr;
	}
}

const FItemInstance* UEquippedStateComponent::GetEquippedItemInternal(EEquipmentSlot Slot) const
{
	switch (Slot)
	{
	case EEquipmentSlot::Primary:
		return &PrimaryWeapon;
	case EEquipmentSlot::Secondary:
		return &SecondaryWeapon;
	case EEquipmentSlot::Pistol:
		return &SidearmWeapon;
	case EEquipmentSlot::Tool:
		return &ToolItem;
	case EEquipmentSlot::Knife:
		return &KnifeItem;
	default:
		return nullptr;
	}
}

bool UEquippedStateComponent::IsValidEquipmentSlot(EEquipmentSlot Slot) const
{
	return Slot != EEquipmentSlot::MAX;
}

bool UEquippedStateComponent::IsSingleItemSlot(EEquipmentSlot Slot) const
{
	switch (Slot)
	{
	case EEquipmentSlot::Primary:
	case EEquipmentSlot::Secondary:
	case EEquipmentSlot::Pistol:
	case EEquipmentSlot::Tool:
	case EEquipmentSlot::Knife:
		return true;
	default:
		return false;
	}
}

float UEquippedStateComponent::GetItemWeight(const FItemInstance& Item) const
{
	if (Item.Definition == nullptr)
	{
		return 0.0f;
	}

	return Item.Definition->Weight * FMath::Max(1, Item.StackCount);
}

void UEquippedStateComponent::BroadcastEquippedStateChanged()
{
	OnEquippedStateChanged.Broadcast();
}

void UEquippedStateComponent::OnRep_PrimaryWeapon()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_SecondaryWeapon()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_SidearmWeapon()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_HelmetItem()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_ChestItem()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_ToolItem()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_KnifeItem()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_CurrentCarriedWeight()
{
	BroadcastEquippedStateChanged();
}

void UEquippedStateComponent::OnRep_CurrentBurdenScore()
{
	BroadcastEquippedStateChanged();
}