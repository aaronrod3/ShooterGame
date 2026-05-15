// Source/ShooterGame/Types/LoadoutTypes.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LoadoutTypes.generated.h"

// ============================================================================
// EEquipmentSlot
//
// The physical slots a player configures in the pregame lobby.
// The integer value of each entry is used as the array index into
// FLoadoutData::Slots — so order matters and gaps are not allowed.
//
// --- HOW TO ADD A NEW SLOT ---
// 1. Add a new entry ABOVE the MAX entry below.
// 2. Do NOT reorder or remove existing entries — this would corrupt saved
//    loadout data for any player who already has a save file, because
//    FLoadoutData serializes Slots as an indexed array.
// 3. After adding, update ULoadoutComponent::CanEquipInSlot() and any
//    UI logic that iterates over slots.
// 4. If adding a visual/unequipped socket slot (e.g. a second tool slot),
//    also add a corresponding socket to the character skeleton and update
//    UItemDefinition::UnequippedSocketName in any affected item assets.
// ============================================================================
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    Primary         UMETA(DisplayName = "Primary Weapon"),
    Secondary       UMETA(DisplayName = "Secondary Weapon"),
    Pistol          UMETA(DisplayName = "Pistol"),
    Tool            UMETA(DisplayName = "Tool"),
    Knife           UMETA(DisplayName = "Knife / Melee"),
    Consumable1     UMETA(DisplayName = "Consumable 1"),
    Consumable2     UMETA(DisplayName = "Consumable 2"),
    SpecialItem     UMETA(DisplayName = "Special Item"),

    // --- ADD NEW SLOTS ABOVE THIS LINE ---
    // MAX must always be last. It is used by FLoadoutData to size the Slots
    // array automatically — adding a new slot above will expand the array.
    MAX             UMETA(Hidden)
};

// ============================================================================
// EWeaponCategory
//
// Describes what type of weapon an item is. This is a descriptor on
// UItemDefinition — it drives AllowedSlots, animation set selection,
// and any category-specific gameplay logic (e.g. suppressor support).
//
// This is NOT the slot — it is the weapon's identity.
// Slot restrictions are data-driven per asset via UItemDefinition::AllowedSlots.
//
// --- HOW TO ADD A NEW WEAPON TYPE ---
// 1. Add a new entry anywhere in the list below (order does not affect saves).
// 2. In any new UItemDefinition asset for that weapon, set AllowedSlots
//    appropriately (e.g. a crossbow might allow Primary or Secondary).
// 3. If the new type needs unique animation logic, handle it in
//    UCombatComponent using a switch on EWeaponCategory.
// ============================================================================
UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
    None            UMETA(DisplayName = "None"),
    Rifle           UMETA(DisplayName = "Assault Rifle"),
    SMG             UMETA(DisplayName = "SMG"),
    LMG             UMETA(DisplayName = "LMG"),
    Shotgun         UMETA(DisplayName = "Shotgun"),
    SniperRifle     UMETA(DisplayName = "Sniper Rifle"),
    GrenadeLauncher UMETA(DisplayName = "Grenade Launcher"),
    Launcher        UMETA(DisplayName = "Launcher / RPG"),
    Pistol          UMETA(DisplayName = "Pistol"),
    Melee           UMETA(DisplayName = "Melee"),
    Tool            UMETA(DisplayName = "Tool"),
    Consumable      UMETA(DisplayName = "Consumable"),
    Ammo            UMETA(DisplayName = "Ammo"),
    SpecialItem     UMETA(DisplayName = "Special Item")

    // --- ADD NEW WEAPON CATEGORIES HERE ---
    // Safe to add at any time. No index dependency.
};

// ============================================================================
// EItemCategory
//
// Broad family grouping used for inventory UI filtering.
// Intentionally NOT split into Primary/Secondary weapon — that distinction
// is handled per-item via UItemDefinition::AllowedSlots, keeping the system
// data-driven and not requiring code changes for new weapon configurations.
//
// --- HOW TO ADD A NEW CATEGORY ---
// 1. Add a new entry below.
// 2. Update any UI filtering logic that switches on EItemCategory.
// 3. If the new category needs unique save/persistence behavior, update
//    UShooterSaveGame and UShooterSaveGameSubsystem accordingly.
// ============================================================================
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
    // --- DO NOT REORDER EXISTING ENTRIES — serialized in existing assets ---
    Weapon          UMETA(DisplayName = "Weapon"),
    Tool            UMETA(DisplayName = "Tool"),
    Melee           UMETA(DisplayName = "Melee"),
    Consumable      UMETA(DisplayName = "Consumable"),
    Ammo            UMETA(DisplayName = "Ammo"),
    SpecialItem     UMETA(DisplayName = "Special Item"),

    // 
    Attachment      UMETA(DisplayName = "Attachment"),      // Weapon mods: scopes, grips, suppressors
    Equipment       UMETA(DisplayName = "Equipment"),       // Helmets, vests, plate carriers
    Medical         UMETA(DisplayName = "Medical"),         // Medkits, bandages, stims
    Intel           UMETA(DisplayName = "Intel"),           // Documents, keycards, drives
    QuestItem       UMETA(DisplayName = "Quest Item"),      // Non-losable mission objective items
    
    // Currency
    Currency        UMETA(DisplayName = "Currency")         // Physical currency items (Dollars)

    // --- ADD NEW ITEM CATEGORIES HERE ---
};

// ============================================================================
// FLoadoutSlot
//
// Represents one configured slot in the player's loadout.
// This struct is replicated and serialized to disk — keep it lean.
//
// ItemClass is a TSoftClassPtr so the asset is not force-loaded into memory
// until the item needs to be spawned. This is critical for a game with many
// weapons — hard references would load every weapon asset at startup.
//
// --- HOW TO EXTEND THIS STRUCT ---
// Adding new fields is safe as long as you give them default values.
// Removing or reordering fields WILL break existing save files — don't do it.
// If you need to version the save format, bump UShooterSaveGame::SaveVersion.
//
// Future additions to consider:
//   - FName AttachmentPreset   (weapon attachment config, e.g. scope/grip)
//   - int32 SkinID             (per-weapon cosmetic skin index)
//   - bool bIsFavorite         (UI pin to top of list)
// ============================================================================
USTRUCT(BlueprintType)
struct FLoadoutSlot
{
    GENERATED_BODY()

    // Soft reference to the weapon/item Blueprint class.
    // Empty (IsNull) = this slot is unoccupied.
    // Use TSoftClassPtr so assets are only loaded when the item is spawned,
    // not at lobby load time.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
    TSoftClassPtr<AActor> ItemClass;

    // How many magazines/charges to bring this round.
    // Weapons: magazine count.   Consumables: use count.
    // Knife / SpecialItem: unused, leave at 0.
    // The valid range per weapon is defined on UItemDefinition::MaxMagazineCount
    // and enforced by ULoadoutComponent::CanEquipInSlot().
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout", meta = (ClampMin = "0"))
    int32 MagazineCount = 0;

    // Optional cosmetic variant tag (e.g. camo, paint job).
    // Resolved by the item's Blueprint — ignored if NAME_None.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
    FName VariantID = NAME_None;

    // Returns true if this slot has an item assigned
    bool IsOccupied() const { return !ItemClass.IsNull(); }

    // Clears this slot back to an empty state
    void Clear() { ItemClass.Reset(); MagazineCount = 0; VariantID = NAME_None; }
};

// ============================================================================
// FLoadoutData
//
// The complete configured loadout for one player — all 8 slots together.
// Replicated as a single struct so one OnRep fires for any loadout change,
// rather than 8 separate rep conditions.
//
// WHY TArray AND NOT TMap:
// TMap is not directly replicable in UE5. We use a fixed TArray indexed by
// (int32)EEquipmentSlot instead. The array always has exactly EEquipmentSlot::MAX
// entries. Adding a new EEquipmentSlot entry automatically expands this array
// via the FLoadoutData constructor — no changes needed here.
//
// SAVE COMPATIBILITY NOTE:
// The array is serialized in index order. Never reorder EEquipmentSlot entries
// or existing save files will map items to the wrong slots.
// ============================================================================
USTRUCT(BlueprintType)
struct FLoadoutData
{
    GENERATED_BODY()

    // Fixed-size array of slots. Index = (int32)EEquipmentSlot.
    // Always has EEquipmentSlot::MAX entries.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loadout")
    TArray<FLoadoutSlot> Slots;

    FLoadoutData()
    {
        // Size the array to exactly match the number of defined slots.
        // If you add a new EEquipmentSlot entry above MAX, this automatically
        // expands — no change needed here.
        Slots.SetNum((int32)EEquipmentSlot::MAX);
    }

    // Returns a mutable reference to the slot for a given equipment slot.
    // Always safe — array is guaranteed to be MAX entries.
    FLoadoutSlot& GetSlot(EEquipmentSlot Slot)
    {
        return Slots[(int32)Slot];
    }

    // Const version for read-only access (e.g. from HUD, replication callbacks)
    const FLoadoutSlot& GetSlot(EEquipmentSlot Slot) const
    {
        return Slots[(int32)Slot];
    }

    // Returns true if at least one slot has an item assigned
    bool HasAnyItems() const
    {
        for (const FLoadoutSlot& Slot : Slots)
        {
            if (Slot.IsOccupied()) return true;
        }
        return false;
    }

    // Clears all slots — used by debug commands and new-player initialization
    void ClearAll()
    {
        for (FLoadoutSlot& Slot : Slots)
        {
            Slot.Clear();
        }
    }
};

// ============================================================================
// FCharacterAppearance
//
// Lightweight cosmetic data replicated in lobby so all players see each
// other's customization choices in real time.
//
// All IDs are integers that index into your character customization data
// asset arrays — avoids string comparisons in gameplay code.
// The actual mesh/material swaps are performed in the character Blueprint
// or a dedicated AppearanceComponent when these values change.
//
// --- HOW TO EXTEND ---
// Adding new int32 or FName fields is safe — give them sensible defaults.
// Future additions to consider:
//   - int32 FaceID             (facial customization)
//   - int32 GloveID            (glove/hand gear)
//   - int32 VestID             (body armor visual tier)
//   - int32 PatchID            (faction/squad emblem)
// ============================================================================
USTRUCT(BlueprintType)
struct FCharacterAppearance
{
    GENERATED_BODY()

    // Index into the character body mesh/skin data asset array.
    // 0 = default skin.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 MeshSkinID = 0;

    // Index into the helmet/headgear data asset array.
    // 0 = no helmet.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 HelmetID = 0;

    // Index into the backpack data asset array.
    // 0 = default backpack.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    int32 BackpackID = 0;

    // Color/camo variant tag resolved per-mesh in the character Blueprint.
    // NAME_None = use the mesh's default material.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Appearance")
    FName ColorVariantID = NAME_None;
};