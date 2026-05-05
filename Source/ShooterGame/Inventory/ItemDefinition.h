// Source/ShooterGame/Inventory/ItemDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ItemDefinition.generated.h"

// ============================================================================
// UItemDefinition
//
// A PrimaryDataAsset that describes one equippable item in the game.
// Create one asset per item (weapon, tool, consumable, etc.) in the editor:
//   Content Browser → Right-click → Miscellaneous → Data Asset → UItemDefinition
//
// This is the single source of truth for an item's identity, slot rules,
// and magazine limits. No weapon names are hardcoded anywhere in C++ —
// adding a new weapon means creating a new asset, zero code changes.
//
// WHY UPrimaryDataAsset:
//   - Registered in the Asset Manager for async streaming via FPrimaryAssetId
//   - Safe to reference via TSoftObjectPtr/TSoftClassPtr from loadout data
//   - Only loaded into memory when needed (lobby screen or item spawn)
//   - Survives cook/packaging correctly with proper AssetManager config
//
// --- HOW TO ADD A NEW ITEM ---
// 1. In the editor: Content Browser → Right-click → Miscellaneous → Data Asset
// 2. Choose UItemDefinition as the class
// 3. Fill in DisplayName, WeaponCategory, ItemCategory, ActorClass, AllowedSlots
// 4. Set UnequippedSocketName to match the socket on the character skeleton
// 5. No C++ changes required
//
// --- HOW TO EXTEND THIS CLASS ---
// Adding new UPROPERTY fields is always safe.
// If you add fields that affect save data, bump UShooterSaveGame::SaveVersion.
// Future fields to consider:
//   - TArray<FName> SupportedAttachments  (valid attachment preset names)
//   - USoundBase* EquipSound              (played when switching to this item)
//   - UAnimMontage* EquipMontage          (override per weapon type)
//   - float CarryWeight                   (for future stamina/carry systems)
//   - int32 UnlockLevel                   (player level required to equip)
// ============================================================================
UCLASS(BlueprintType)
class SHOOTERGAME_API UItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Identity
    // -----------------------------------------------------------------------

    // Human-readable display name shown in the lobby loadout UI.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Identity")
    FText DisplayName;

    // Short description shown in item tooltip / inspect UI.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Identity")
    FText Description;

    // Broad family this item belongs to — used for UI filtering.
    // Does NOT control slot placement; use AllowedSlots for that.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Identity")
    EItemCategory ItemCategory = EItemCategory::Weapon;

    // Specific weapon/item type — drives animation set selection and
    // any category-specific gameplay logic in UCombatComponent.
    // Set to None for non-weapon items (tools, consumables, special items).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Identity")
    EWeaponCategory WeaponCategory = EWeaponCategory::None;

    // Icon shown in the lobby loadout grid and HUD quickbar.
    // Use a 256x256 or 512x512 texture for clean display at all UI scales.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Identity")
    TObjectPtr<UTexture2D> ThumbnailIcon;

    // -----------------------------------------------------------------------
    // Spawning
    // -----------------------------------------------------------------------

    // The weapon/item Blueprint class to spawn when this item is equipped.
    // Soft reference — not loaded until the item is actually spawned.
    // Point this at your AWeapon-derived Blueprint (e.g. BP_Rifle_AK47).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Spawning")
    TSoftClassPtr<AActor> ActorClass;

    // -----------------------------------------------------------------------
    // Slot Rules
    // -----------------------------------------------------------------------

    // Which equipment slots this item is allowed to occupy.
    // Enforced by ULoadoutComponent::CanEquipInSlot() at loadout-set time.
    //
    // Standard configurations:
    //   Rifle/SMG/LMG/Shotgun  → {Primary, Secondary}
    //   SniperRifle/Launcher   → {Secondary}
    //   Pistol                 → {Pistol}
    //   Knife                  → {Knife}
    //   Tools                  → {Tool}
    //   Consumables            → {Consumable1, Consumable2}
    //   Special Items          → {SpecialItem}
    //
    // Leave empty to block equipping this item entirely (useful for
    // quest items that only exist in SpecialItem via code, not player choice).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Slot Rules")
    TArray<EEquipmentSlot> AllowedSlots;

    // -----------------------------------------------------------------------
    // Magazine / Charge Rules
    // -----------------------------------------------------------------------

    // Maximum magazines (weapons) or uses (consumables) the player can
    // choose to bring per round. The lobby UI clamps the player's choice
    // to this value. Can be raised by player class progression later.
    // Set to 0 for items that don't use a count (knife, special items).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Ammo",
        meta = (ClampMin = "0"))
    int32 MaxMagazineCount = 4;

    // Default magazine count pre-filled in the lobby UI for new players.
    // Should be <= MaxMagazineCount.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Ammo",
        meta = (ClampMin = "0"))
    int32 DefaultMagazineCount = 2;

    // -----------------------------------------------------------------------
    // Unequipped Visual Placement
    // -----------------------------------------------------------------------

    // The socket name on the character skeleton where this item attaches
    // when it is in the loadout but NOT the active weapon.
    //
    // Standard socket names (add these to the character skeleton in editor):
    //   Primary weapon on back    → "Socket_Primary_Back"
    //   Secondary on backpack     → "Socket_Secondary_Back"
    //   Pistol in holster         → "Socket_Pistol_Holster"
    //   Tool on backpack          → "Socket_Tool_Back"
    //   Knife on belt             → "Socket_Knife_Belt"
    //   Grenades on belt          → "Socket_Grenade_Belt"
    //
    // Leave as NAME_None to skip visual attachment for this item
    // (e.g. consumables that have no world representation when unequipped).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Visual")
    FName UnequippedSocketName = NAME_None;

    // -----------------------------------------------------------------------
    // Asset Manager Integration
    // -----------------------------------------------------------------------

    // Returns the PrimaryAssetId used by the Asset Manager to track and
    // async-load this definition. Format: "ItemDefinition:AssetName"
    // Required for TSoftObjectPtr streaming to work correctly at runtime.
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};