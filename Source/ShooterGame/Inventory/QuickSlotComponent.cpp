// Source/ShooterGame/Inventory/QuickSlotComponent.cpp
#include "Inventory/QuickSlotComponent.h"

#include "Net/UnrealNetwork.h"

UQuickSlotComponent::UQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultSlots();
}

void UQuickSlotComponent::InitializeDefaultSlots()
{
	if (QuickSlotAssignments.Num() == MaxQuickSlots)
	{
		return;
	}

	QuickSlotAssignments.Reset();
	for (int32 SlotIndex = 0; SlotIndex < MaxQuickSlots; ++SlotIndex)
	{
		FQuickSlotAssignment Assignment;
		Assignment.SlotIndex = SlotIndex;
		QuickSlotAssignments.Add(Assignment);
	}
}

void UQuickSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UQuickSlotComponent, QuickSlotAssignments);
}

bool UQuickSlotComponent::Server_AssignQuickSlot(int32 SlotIndex, EQuickSlotCategory Category, const FGuid& ItemInstanceID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!IsValidQuickSlotIndex(SlotIndex) || !ItemInstanceID.IsValid())
	{
		return false;
	}

	FQuickSlotAssignment* Assignment = FindAssignmentMutable(SlotIndex);
	if (!Assignment)
	{
		return false;
	}

	Assignment->SlotIndex = SlotIndex;
	Assignment->Category = Category;
	Assignment->ItemInstanceID = ItemInstanceID;

	BroadcastQuickSlotsChanged();
	return true;
}

bool UQuickSlotComponent::Server_ClearQuickSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	FQuickSlotAssignment* Assignment = FindAssignmentMutable(SlotIndex);
	if (!Assignment)
	{
		return false;
	}

	Assignment->SlotIndex = SlotIndex;
	Assignment->Category = EQuickSlotCategory::None;
	Assignment->ItemInstanceID.Invalidate();

	BroadcastQuickSlotsChanged();
	return true;
}

void UQuickSlotComponent::Server_ClearAllQuickSlots()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < QuickSlotAssignments.Num(); ++SlotIndex)
	{
		QuickSlotAssignments[SlotIndex].SlotIndex = SlotIndex;
		QuickSlotAssignments[SlotIndex].Category = EQuickSlotCategory::None;
		QuickSlotAssignments[SlotIndex].ItemInstanceID.Invalidate();
	}

	BroadcastQuickSlotsChanged();
}

bool UQuickSlotComponent::GetQuickSlotAssignment(int32 SlotIndex, FQuickSlotAssignment& OutAssignment) const
{
	if (const FQuickSlotAssignment* Assignment = FindAssignment(SlotIndex))
	{
		OutAssignment = *Assignment;
		return true;
	}

	return false;
}

TArray<FQuickSlotAssignment> UQuickSlotComponent::GetAllQuickSlotAssignments() const
{
	return QuickSlotAssignments;
}

bool UQuickSlotComponent::IsValidQuickSlotIndex(int32 SlotIndex) const
{
	return SlotIndex >= 0 && SlotIndex < MaxQuickSlots;
}

void UQuickSlotComponent::OnRep_QuickSlotAssignments()
{
	BroadcastQuickSlotsChanged();
}

void UQuickSlotComponent::BroadcastQuickSlotsChanged()
{
	OnQuickSlotsChanged.Broadcast();
}

FQuickSlotAssignment* UQuickSlotComponent::FindAssignmentMutable(int32 SlotIndex)
{
	for (FQuickSlotAssignment& Assignment : QuickSlotAssignments)
	{
		if (Assignment.SlotIndex == SlotIndex)
		{
			return &Assignment;
		}
	}

	return nullptr;
}

const FQuickSlotAssignment* UQuickSlotComponent::FindAssignment(int32 SlotIndex) const
{
	for (const FQuickSlotAssignment& Assignment : QuickSlotAssignments)
	{
		if (Assignment.SlotIndex == SlotIndex)
		{
			return &Assignment;
		}
	}

	return nullptr;
}