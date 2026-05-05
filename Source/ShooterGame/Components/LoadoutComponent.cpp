// Source/ShooterGame/Components/LoadoutComponent.cpp


#include "LoadoutComponent.h"
#include "Net/UnrealNetwork.h"
#include "ShooterGame/Inventory/ItemDefinition.h"

ULoadoutComponent::ULoadoutComponent()
{
    // This component does not need Tick — all updates are event-driven via RPCs
    // and rep notifies. Enable if you add time-based loadout logic later.
    PrimaryComponentTick.bCanEverTick = false;

    // Must be true for UPROPERTY(Replicated) to work on a component
    SetIsReplicatedByDefault(true);
}

void ULoadoutComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate to all clients — every player needs to see everyone's
    // loadout in lobby, and the owning client needs it for HUD
    DOREPLIFETIME(ULoadoutComponent, LoadoutData);
    DOREPLIFETIME(ULoadoutComponent, Appearance);
}

void ULoadoutComponent::BeginPlay()
{
    Super::BeginPlay();
    // Future: if you need to initialize default loadout from a data table
    // or player class definition, do it here on the server only:
    //   if (IsServer()) { ApplyDefaultLoadoutForClass(PlayerClass); }
}

// ============================================================================
// Public Interface
// ============================================================================

void ULoadoutComponent::Server_SetSlot(EEquipmentSlot Slot, const FLoadoutSlot& NewSlotData)
{
    // All loadout mutations must be server-authoritative
    if (!IsServer()) return;

    // If the slot is being cleared (empty ItemClass), allow it without validation
    if (NewSlotData.IsOccupied())
    {
        // Load the item definition synchronously for validation.
        // This is lobby-only so a blocking load is acceptable here —
        // if this ever moves to in-game, swap for an async load.
        UClass* ResolvedClass = NewSlotData.ItemClass.LoadSynchronous();
        UItemDefinition* ItemDef = ResolvedClass
            ? Cast<UItemDefinition>(ResolvedClass->GetDefaultObject())
            : nullptr;

        if (!CanEquipInSlot(ItemDef, Slot))
        {
            UE_LOG(LogTemp, Warning,
                TEXT("ULoadoutComponent::Server_SetSlot — item not allowed in slot %d. Assignment rejected."),
                (int32)Slot);
            return;
        }
    }

    LoadoutData.GetSlot(Slot) = NewSlotData;

    // Manually fire the rep notify on the server — clients get it via replication,
    // but the server runs OnRep separately since it doesn't receive its own rep
    OnRep_LoadoutData();
}

void ULoadoutComponent::Server_SetFullLoadout(const FLoadoutData& NewLoadout)
{
    // Server-only — called by AShooterPlayerState when pushing saved data on spawn
    if (!IsServer()) return;

    LoadoutData = NewLoadout;

    // Fire once after full replacement rather than once per slot
    OnRep_LoadoutData();
}

void ULoadoutComponent::Server_SetAppearance(const FCharacterAppearance& NewAppearance)
{
    if (!IsServer()) return;

    Appearance = NewAppearance;
    OnRep_Appearance();
}

void ULoadoutComponent::BroadcastCurrentLoadout()
{
    // Useful after bulk-loading to ensure all listeners (CombatComponent, HUD)
    // are in sync with the current state even if no rep delta occurred
    OnLoadoutChanged.Broadcast(LoadoutData);
}

// ============================================================================
// Validation
// ============================================================================

bool ULoadoutComponent::CanEquipInSlot(UItemDefinition* ItemDef, EEquipmentSlot Slot) const
{
    if (!ItemDef)
    {
        return false;
    }

    if (ItemDef->AllowedSlots.IsEmpty())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("ULoadoutComponent::CanEquipInSlot — ItemDef '%s' has no AllowedSlots configured."),
            *ItemDef->GetName());
        return false;
    }

    // Check if the requested slot is in the item's allowed list
    return ItemDef->AllowedSlots.Contains(Slot);

    // --- EXTEND VALIDATION HERE ---
    // Future checks to add after the Contains() call:
    //
    // Check player class restriction:
    //   if (ItemDef->RequiredClass != EPlayerClass::Any &&
    //       ItemDef->RequiredClass != OwnerPlayerClass) return false;
    //
    // Check unlock level:
    //   if (ItemDef->UnlockLevel > OwnerPlayerLevel) return false;
    //
    // Check team-unique item limits:
    //   if (ItemDef->bIsUniquePerTeam && TeamAlreadyHasItem(ItemDef)) return false;
}

// ============================================================================
// Rep Notifies
// ============================================================================

void ULoadoutComponent::OnRep_LoadoutData()
{
    // Broadcast to all locally bound listeners (CombatComponent, HUD, etc.)
    // This fires on clients via replication and on the server via manual call
    // inside Server_SetSlot / Server_SetFullLoadout
    OnLoadoutChanged.Broadcast(LoadoutData);
}

void ULoadoutComponent::OnRep_Appearance()
{
    OnAppearanceChanged.Broadcast(Appearance);
}

// ============================================================================
// Internal Helpers
// ============================================================================

bool ULoadoutComponent::IsServer() const
{
    // GetOwner() is always valid during gameplay — safe to call after BeginPlay
    return GetOwner() && GetOwner()->HasAuthority();
}