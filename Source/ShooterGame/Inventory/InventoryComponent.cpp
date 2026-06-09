// Source/ShooterGame/Inventory/InventoryComponent.cpp


#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "ShooterGame/Inventory/ItemDefinition.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, Items);
	DOREPLIFETIME(UInventoryComponent, MagazineSlots);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

// -----------------------------------------------------------------------
// OnRep Handlers
// -----------------------------------------------------------------------

void UInventoryComponent::OnRep_Items()
{
	// Broadcast full refresh — UI widgets rebuild from GetItems()
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::OnRep_MagazineSlots()
{
	// Legacy shim — no UI delegate needed here until ammo migrates to FItemInstance
}

// -----------------------------------------------------------------------
// Slot Tag Validation
// -----------------------------------------------------------------------

bool UInventoryComponent::IsItemValidForSlot(const FItemInstance& Item, const FGameplayTag& RequiredSlotTag)
{
	// An empty RequiredSlotTag means the container accepts anything (e.g. stash)
	if (!RequiredSlotTag.IsValid()) return true;

	if (!Item.Definition) return false;

	// Item is valid if its AcceptedSlotTags contains the slot's required tag
	return Item.Definition->AcceptedSlotTags.HasTag(RequiredSlotTag);
}

// -----------------------------------------------------------------------
// Server-Authoritative Mutations
// -----------------------------------------------------------------------

bool UInventoryComponent::Server_AddItem(FItemInstance& ItemInstance, const FGameplayTag& SlotTag)
{
	// Server authority check — clients must never mutate directly
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	// Capacity check (MaxItems == 0 means unlimited)
	if (MaxItems > 0 && Items.Num() >= MaxItems) return false;

	// Slot tag compatibility check
	if (!IsItemValidForSlot(ItemInstance, SlotTag)) return false;

	// Assign a fresh InstanceID if this is a newly created item
	if (!ItemInstance.InstanceID.IsValid())
	{
		ItemInstance.AssignNewInstanceID();
	}

	// Mirror the quest flag from Definition for fast runtime checks
	if (ItemInstance.Definition)
	{
		ItemInstance.bIsQuestItem = ItemInstance.Definition->bIsQuestItem;
	}

	Items.Add(ItemInstance);

	// Mark dirty for replication — OnRep_Items fires on clients
	GetOwner()->ForceNetUpdate();

	return true;
}

bool UInventoryComponent::Server_RemoveItem(const FGuid& InstanceID, FItemInstance& OutItem)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	const int32 Index = Items.IndexOfByPredicate([&InstanceID](const FItemInstance& Inst)
	{
		return Inst.InstanceID == InstanceID;
	});

	if (Index == INDEX_NONE) return false;

	OutItem = Items[Index];
	Items.RemoveAt(Index);

	GetOwner()->ForceNetUpdate();
	return true;
}

bool UInventoryComponent::Server_TransferItemTo(const FGuid& InstanceID, UInventoryComponent* TargetInventory, const FGameplayTag& TargetSlotTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;
	if (!TargetInventory) return false;

	// Pull the item out of this inventory first
	FItemInstance ItemToTransfer;
	if (!Server_RemoveItem(InstanceID, ItemToTransfer)) return false;

	// Attempt to add to target — if target rejects, return the item here (atomic)
	if (!TargetInventory->Server_AddItem(ItemToTransfer, TargetSlotTag))
	{
		// Target rejected — put item back so it is never lost
		FGameplayTag NoTag;
		Server_AddItem(ItemToTransfer, NoTag);
		return false;
	}

	return true;
}

bool UInventoryComponent::Server_UpdateItemState(const FGuid& InstanceID, const FItemInstance& UpdatedInstance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return false;

	FItemInstance* Found = FindItemByID_Mutable(InstanceID);
	if (!Found) return false;

	// Only update runtime state — never overwrite Definition or InstanceID
	Found->Durability       = UpdatedInstance.Durability;
	Found->MaxCondition     = UpdatedInstance.MaxCondition;
	Found->StackCount       = UpdatedInstance.StackCount;
	Found->LoadedAmmoType   = UpdatedInstance.LoadedAmmoType;
	Found->LoadedAmmoCount  = UpdatedInstance.LoadedAmmoCount;
	Found->AttachedMods     = UpdatedInstance.AttachedMods;
	Found->WeaponParts      = UpdatedInstance.WeaponParts;

	GetOwner()->ForceNetUpdate();

	// Notify UI of the targeted state change — lightweight single-slot refresh
	OnItemStateChanged.Broadcast(InstanceID);

	return true;
}

// -----------------------------------------------------------------------
// Read-Only Accessors
// -----------------------------------------------------------------------

const FItemInstance* UInventoryComponent::FindItemByID(const FGuid& InstanceID) const
{
	return Items.FindByPredicate([&InstanceID](const FItemInstance& Inst)
	{
		return Inst.InstanceID == InstanceID;
	});
}

FItemInstance* UInventoryComponent::FindItemByID_Mutable(const FGuid& InstanceID)
{
	return Items.FindByPredicate([&InstanceID](const FItemInstance& Inst)
	{
		return Inst.InstanceID == InstanceID;
	});
}

bool UInventoryComponent::ContainsItem(const FGuid& InstanceID) const
{
	return FindItemByID(InstanceID) != nullptr;
}

bool UInventoryComponent::ContainsDefinition(const UItemDefinition* Definition) const
{
	if (!Definition) return false;

	return Items.ContainsByPredicate([Definition](const FItemInstance& Inst)
	{
		return Inst.Definition == Definition;
	});
}

bool UInventoryComponent::HasCapacity() const
{
	// MaxItems == 0 means unlimited (stash)
	if (MaxItems == 0) return true;
	return Items.Num() < MaxItems;
}

// -----------------------------------------------------------------------
// Magazine Shim — delegates to legacy MagazineSlots array
// Behavior is identical to the original InventoryComponent implementation.
// -----------------------------------------------------------------------

bool UInventoryComponent::AddMagazine(const FMagazine& Mag)
{
	if (MagazineSlots.Num() >= MaxMagazineSlots) return false;
	MagazineSlots.Add(Mag);
	GetOwner()->ForceNetUpdate();
	return true;
}

FMagazine* UInventoryComponent::GetBestMagazine(EAmmoType AmmoType)
{
	FMagazine* Best = nullptr;
	for (FMagazine& Mag : MagazineSlots)
	{
		if (Mag.AmmoType == AmmoType && !Mag.IsEmpty())
		{
			if (!Best || Mag.CurrentRounds > Best->CurrentRounds)
			{
				Best = &Mag;
			}
		}
	}
	return Best;
}

bool UInventoryComponent::RemoveMagazine(const FMagazine& Mag)
{
	const int32 Index = MagazineSlots.IndexOfByKey(Mag);
	if (Index == INDEX_NONE) return false;
	MagazineSlots.RemoveAt(Index);
	GetOwner()->ForceNetUpdate();
	return true;
}

bool UInventoryComponent::ReturnMagazine(const FMagazine& Mag)
{
	if (Mag.IsEmpty()) return false;
	return AddMagazine(Mag);
}

int32 UInventoryComponent::CountMagazinesOfType(EAmmoType AmmoType) const
{
	int32 Count = 0;
	for (const FMagazine& Mag : MagazineSlots)
	{
		if (Mag.AmmoType == AmmoType && !Mag.IsEmpty()) ++Count;
	}
	return Count;
}

bool UInventoryComponent::HasMagazineFor(EAmmoType AmmoType) const
{
	return CountMagazinesOfType(AmmoType) > 0;
}

void UInventoryComponent::ClearAllMagazines()
{
	MagazineSlots.Empty();
}