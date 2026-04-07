// Source/ShooterGame/Components/InventoryComponent.cpp

#include "InventoryComponent.h"
#include "Net/UnrealNetwork.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UInventoryComponent, MagazineSlots);
}

// -----------------------------------------------------------------------
// Magazine Slot Interface
// -----------------------------------------------------------------------

bool UInventoryComponent::AddMagazine(const FMagazine& Mag)
{
	if (MagazineSlots.Num() >= MaxMagazineSlots)
	{
		UE_LOG(LogTemp, Warning, TEXT("UInventoryComponent::AddMagazine — Slots full (%d/%d)"),
			MagazineSlots.Num(), MaxMagazineSlots);
		return false;
	}

	MagazineSlots.Add(Mag);
	UE_LOG(LogTemp, Log, TEXT("UInventoryComponent::AddMagazine — Added %d/%d rounds (%d slots used)"),
		Mag.CurrentRounds, Mag.Capacity, MagazineSlots.Num());
	return true;
}

FMagazine* UInventoryComponent::GetBestMagazine(EAmmoType AmmoType)
{
	FMagazine* Best = nullptr;

	for (FMagazine& Mag : MagazineSlots)
	{
		// Skip wrong caliber or empty mags
		if (Mag.AmmoType != AmmoType || Mag.IsEmpty()) continue;

		// Prefer the fullest magazine available
		if (!Best || Mag.CurrentRounds > Best->CurrentRounds)
		{
			Best = &Mag;
		}
	}

	return Best;
}

bool UInventoryComponent::RemoveMagazine(const FMagazine& Mag)
{
	int32 Idx = MagazineSlots.IndexOfByKey(Mag);
	if (Idx == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("UInventoryComponent::RemoveMagazine — Magazine not found in slots"));
		return false;
	}

	MagazineSlots.RemoveAt(Idx);
	return true;
}

bool UInventoryComponent::ReturnMagazine(const FMagazine& Mag)
{
	// Discard completely empty mags — no point holding a slot for them
	if (Mag.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("UInventoryComponent::ReturnMagazine — Discarding empty mag"));
		return false;
	}

	// If slots are full, the partial mag is lost (dropped on the ground — future feature)
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
	for (const FMagazine& Mag : MagazineSlots)
	{
		if (Mag.AmmoType == AmmoType && !Mag.IsEmpty()) return true;
	}
	return false;
}

void UInventoryComponent::OnRep_MagazineSlots()
{
	// TODO: broadcast a delegate here so the HUD can refresh the ammo/inventory display
	// Example: OnInventoryUpdated.Broadcast();
}