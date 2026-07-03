// Source/ShooterGame/Types/ItemTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShooterGame/Types/AmmoType.h"
#include "ItemTypes.generated.h"

// ============================================================================
// EItemRepairMethod
//
// Defines what action at the gunsmith/workbench restores this item's Durability.
// Set once on UItemDefinition — drives UI label and animation selection.
// Each weapon/item type has a single canonical repair method.
//
// Examples:
//   Rifle, Pistol, SMG  → Clean       (solvent, brush, oil)
//   Knife, Machete      → Sharpen     (whetstone)
//   Armor, Vest         → ServiceKit  (patch kit, needle+thread)
//   Consumable/Medical  → FieldRepair (improvised, no bench needed)
// ============================================================================
UENUM(BlueprintType)
enum class EItemRepairMethod : uint8
{
    None            UMETA(DisplayName = "None"),            // Item does not degrade (quest items, intel)
    Clean           UMETA(DisplayName = "Clean"),           // Firearms: solvent + oil at gunsmith bench
    Sharpen         UMETA(DisplayName = "Sharpen"),         // Bladed melee: whetstone at bench
    ServiceKit      UMETA(DisplayName = "Service Kit"),     // Armor/gear: patch kit at bench
    FieldRepair     UMETA(DisplayName = "Field Repair")     // Consumables: repaired anywhere, no bench
};


// ============================================================================
// FWeaponPartCondition
//
// DORMANT FORWARD-COMPATIBILITY STUB — do not activate until per-part
// weapon infrastructure (barrel, bolt, receiver actors) exists.
//
// This struct is intentionally kept alive but empty-by-default so that
// FItemInstance::WeaponParts can be declared now without any rewrite later.
// When per-part durability is activated:
//   1. Populate WeaponParts on weapon FItemInstances at spawn time
//   2. Route per-shot degradation to the relevant part instead of FItemInstance::Durability
//   3. FItemInstance::Durability becomes the aggregate/display value
//
// PartName examples (future): "Barrel", "Bolt", "Receiver", "Gas_Block"
// ============================================================================
USTRUCT(BlueprintType)
struct FWeaponPartCondition
{
    GENERATED_BODY()

    // Identifies which part of the weapon this entry tracks.
    // NAME_None = uninitialized (dormant stub state).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Parts")
    FName PartName = NAME_None;

    // Current condition of this specific part (0.0 – 100.0).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Parts",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float PartDurability = 100.0f;

    // Permanent max condition ceiling for this part — degrades with extended use.
    // Once MaxPartCondition drops, cleaning can only restore up to this ceiling.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Parts",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float MaxPartCondition = 100.0f;
};


// ============================================================================
// FAttachedModInstance
//
// A weapon attachment (scope, grip, suppressor, muzzle device) mounted on a
// specific weapon instance. Intentionally a separate struct — not FItemInstance
// — because UHT forbids USTRUCT self-recursion via TArray.
//
// Attachments are committed pregame (gunsmith is lobby-only). They cannot be
// swapped in-mission. Each mod tracks its own InstanceID and Durability so it
// can be independently transferred to stash after extraction.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FAttachedModInstance
{
    GENERATED_BODY()

    // Pointer to the mod's static definition (scope, suppressor, grip, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment")
    TObjectPtr<class UItemDefinition> Definition = nullptr;

    // Unique identity of this specific mod instance — stable across transfers.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FGuid InstanceID;

    // Current condition of this mod (0.0–100.0).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float Durability = 100.0f;

    // Permanent condition ceiling — degrades with long-term use.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attachment",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float MaxCondition = 100.0f;

    bool IsValid() const { return Definition != nullptr && InstanceID.IsValid(); }
    void AssignNewInstanceID() { InstanceID = FGuid::NewGuid(); }
};




// ============================================================================
// FItemInstance
//
// The runtime state of one item in the world — what a player actually owns,
// carries, finds, or loses. Every item the player interacts with at runtime
// is an FItemInstance.
//
// ARCHITECTURE NOTES:
//   - Definition is the shared static blueprint (UItemDefinition data asset).
//     Multiple FItemInstances can share one Definition — they differ by runtime
//     state (durability, stack count, mods, ammo loaded, etc.)
//   - InstanceID is a server-generated GUID. It is the canonical identity of
//     this item across stash, carried state, and mission containers.
//     The client UI always references items by InstanceID — never by array index.
//   - AttachedMods is a flat list of weapon attachment instances inline on the
//     weapon. Mods are committed pregame and cannot be swapped in-mission.
//   - WeaponParts is dormant (always empty) until per-part durability activates.
//     It is declared here so FItemInstance never needs a breaking change later.
//   - bIsQuestItem mirrors the Definition flag for fast replication checks
//     without dereferencing the soft pointer at runtime.
//
// REPLICATION:
//   FItemInstance is designed to be held in a TArray inside UInventoryComponent
//   which is replicated via OnRep. The struct itself is not a UObject and does
//   not self-replicate — the owning component drives all replication.
//
// DURABILITY MODEL:
//   Durability      = current condition (0.0–100.0). Decreases with use.
//   MaxCondition    = permanent ceiling (starts 100.0). Slowly degrades with
//                     extended use — cleaning restores Durability toward this
//                     ceiling but cannot exceed it.
//   RepairRestoreAmount (on Definition) = how much Durability is restored
//                     per repair action. Set per-item in editor.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FItemInstance
{
    GENERATED_BODY()

    // -----------------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------------

    // Pointer to the shared static item definition.
    // Null = invalid/empty instance. Always check IsValid() before use.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    TObjectPtr<UItemDefinition> Definition = nullptr;

    // Server-generated unique identifier for this specific item instance.
    // Stable across transfer between containers (stash ↔ rig ↔ mission cache).
    // The UI references items by InstanceID — never by TArray index.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    FGuid InstanceID;

    // -----------------------------------------------------------------------
    // Stack
    // -----------------------------------------------------------------------

    // How many units this instance represents (1 for non-stackable items).
    // Clamped to Definition->StackMax at transfer time by the server.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item",
        meta = (ClampMin = "1"))
    int32 StackCount = 1;

    // -----------------------------------------------------------------------
    // Durability
    // -----------------------------------------------------------------------

    // Current condition of this item (0.0 = broken, 100.0 = perfect).
    // Decreases with use (shots fired, damage taken, etc.).
    // Restored by repair actions, capped at MaxCondition.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float Durability = 100.0f;

    // Permanent condition ceiling. Starts at 100.0 and slowly decreases
    // with extended use — models long-term wear that cannot be fully reversed.
    // Cleaning/repair restores Durability toward MaxCondition, not beyond it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float MaxCondition = 100.0f;

    // -----------------------------------------------------------------------
    // Per-Part Durability (DORMANT — forward-compat stub)
    // -----------------------------------------------------------------------

    // Empty by default. Populated by weapon part infrastructure in a future phase.
    // Declaring it here ensures FItemInstance never needs a structural rewrite.
    // See FWeaponPartCondition for activation notes.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Durability")
    TArray<FWeaponPartCondition> WeaponParts;

    // -----------------------------------------------------------------------
    // Weapon Attachments
    // -----------------------------------------------------------------------

    // Inline attachment list — mods physically mounted on this weapon instance.
    // Uses FAttachedModInstance instead of FItemInstance to avoid USTRUCT
    // self-recursion, which UHT does not support.
    // Non-weapon items leave this empty.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Attachments")
    TArray<FAttachedModInstance> AttachedMods;

    // -----------------------------------------------------------------------
    // Loaded Ammo (weapons only)
    // -----------------------------------------------------------------------

    // Caliber currently loaded in this weapon's chamber/magazine.
    // EAmmoType::EAT_MAX = no ammo loaded / not a weapon.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Ammo")
    EAmmoType LoadedAmmoType = EAmmoType::EAT_MAX;

    // Rounds currently loaded in this weapon instance.
    // 0 = empty chamber. Managed by UCombatComponent during a mission.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item|Ammo",
        meta = (ClampMin = "0"))
    int32 LoadedAmmoCount = 0;

    // -----------------------------------------------------------------------
    // Flags
    // -----------------------------------------------------------------------

    // Mirrors Definition->bIsQuestItem for fast server-side validation
    // without dereferencing the Definition pointer during replication callbacks.
    // Set when the instance is created — never changed at runtime.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
    bool bIsQuestItem = false;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Returns true if this instance has a valid Definition assigned.
    bool IsValid() const { return Definition != nullptr && InstanceID.IsValid(); }

    // Returns true if this item is a weapon (has a WeaponCategory != None).
    // Requires a valid Definition — always call IsValid() first.
    bool IsWeapon() const;

    // Returns true if Durability is at or below the broken threshold.
    bool IsBroken() const { return Durability <= 0.0f; }

    // Clamps Durability to [0, MaxCondition] — call after any durability mutation.
    void ClampDurability() { Durability = FMath::Clamp(Durability, 0.0f, MaxCondition); }

    // Applies a repair action: restores Durability by RepairAmount, capped at MaxCondition.
    // Does NOT modify MaxCondition — that degrades separately via ApplyLongTermWear().
    void ApplyRepair(float RepairAmount)
    {
        Durability = FMath::Min(Durability + RepairAmount, MaxCondition);
    }

    // Applies long-term wear to MaxCondition (called periodically, not per-shot).
    // WearAmount is typically a very small value (e.g. 0.05 per mission completed).
    void ApplyLongTermWear(float WearAmount)
    {
        MaxCondition = FMath::Max(MaxCondition - WearAmount, 0.0f);
        ClampDurability(); // Ensure Durability doesn't exceed new MaxCondition
    }

    // Generates a new server-side InstanceID. Call once on the server when
    // an item is first created (loot spawn, vendor purchase, mission reward).
    // Never call on the client.
    void AssignNewInstanceID() { InstanceID = FGuid::NewGuid(); }

    // Equality by InstanceID only — two instances are the "same item" if their
    // GUIDs match, regardless of current runtime state (durability, ammo, etc.)
    bool operator==(const FItemInstance& Other) const { return InstanceID == Other.InstanceID; }
    bool operator!=(const FItemInstance& Other) const { return InstanceID != Other.InstanceID; }
};