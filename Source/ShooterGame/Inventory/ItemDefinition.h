// Source/ShooterGame/Inventory/ItemDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"                    
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ShooterGame/Types/ItemTypes.h"             
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
    // Slot Compatibility (Phase 1 addition)
    // -----------------------------------------------------------------------

    // GameplayTags this item is valid for. An item can carry multiple tags
    // simultaneously, making it valid in multiple container types at once.
    //
    // A container/slot exposes one FGameplayTag as its RequiredSlotTag.
    // This item is accepted if AcceptedSlotTags.HasTag(RequiredSlotTag).
    //
    // Standard tags (defined in ShooterGameplayTags.h and Config/DefaultGameplayTags.ini):
    //   Slot.Backpack   — general storage
    //   Slot.Rig        — tactical rig / chest rig slots
    //   Slot.Belt       — belt pouches
    //   Slot.WeaponMod  — weapon attachment socket
    //   Slot.Holster    — pistol holster
    //   Slot.MedPouch   — dedicated medical pouch
    //
    // Example: A pistol magazine might have tags {Slot.Backpack, Slot.Rig, Slot.Belt}
    // so it is valid in any of those three container types.
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Slot Compatibility")
    FGameplayTag ItemTypeTag;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Slot Compatibility")
    FGameplayTagContainer AcceptedSlotTags;

    // -----------------------------------------------------------------------
    // Weight & Stacking (Phase 1 addition)
    // -----------------------------------------------------------------------

    // Weight of one unit/stack of this item in kilograms.
    // Does NOT block placement — only affects runtime character movement behavior.
    // Weight system driven by skill tree in a future phase.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Weight & Stack",
        meta = (ClampMin = "0.0"))
    float Weight = 0.1f;

    // Maximum units that can be combined into one FItemInstance stack.
    // 1 = non-stackable (weapons, armor, unique items).
    // > 1 = stackable (ammo boxes, bandages, consumables).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Weight & Stack",
        meta = (ClampMin = "1"))
    int32 StackMax = 1;

    // -----------------------------------------------------------------------
    // Quest Item Flag (Phase 1 addition)
    // -----------------------------------------------------------------------

    // If true, this item is a quest item:
    //   - Never at risk on death (cannot be in carried gear that gets lost)
    //   - Automatically counts toward quest objectives if already in stash
    //   - Cannot be sold to vendors
    //   - Highlighted differently in UI
    // Mirrored onto FItemInstance::bIsQuestItem at instance creation time
    // for fast server-side checks without dereferencing the Definition.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Quest")
    bool bIsQuestItem = false;

    // -----------------------------------------------------------------------
    // Vendor Pricing (Phase 1 addition)
    // -----------------------------------------------------------------------

    // Price the vendor charges to sell this item to the player (player buys).
    // 0 = not available for purchase from vendors.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Vendor",
        meta = (ClampMin = "0"))
    int32 VendorBuyPrice = 0;

    // Price the vendor pays when the player sells this item (player sells).
    // 0 = not sellable (quest items, special items).
    // Typically lower than VendorBuyPrice — vendors profit on the spread.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Vendor",
        meta = (ClampMin = "0"))
    int32 VendorSellPrice = 0;

    // -----------------------------------------------------------------------
    // Durability / Repair (Phase 1 addition)
    // -----------------------------------------------------------------------

    // What bench action restores this item's Durability.
    // None = this item does not degrade (intel docs, quest items).
    // Drives UI label ("Clean", "Sharpen", etc.) and bench animation selection.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Durability")
    EItemRepairMethod RepairMethod = EItemRepairMethod::None;

    // How much Durability is restored per single repair action at the bench.
    // Example: 25.0 means one cleaning session restores 25 points toward MaxCondition.
    // Tune this per item in the editor — high-quality weapons restore more per clean.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Durability",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float RepairRestoreAmount = 25.0f;

    // How much MaxCondition degrades per mission completed (long-term wear rate).
    // Very small values recommended (e.g. 0.5 = loses 0.5 MaxCondition per mission).
    // 0.0 = this item never permanently degrades (tools, quest items).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Durability",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float LongTermWearRate = 0.5f;
    
    
    // -----------------------------------------------------------------------
    // Currency
    // -----------------------------------------------------------------------

    // If true, this item is a physical currency (Rubles, Dollars, Gold, etc.).
    // Currency items:
    //   - Live exclusively in the stash — never carried into missions via loadout
    //   - Are consumed by ServerPurchaseItem when the player buys from a vendor
    //   - Are awarded by ServerSellItem when the player sells to a vendor
    //   - Stack up to StackMax units per FItemInstance (set StackMax generously, e.g. 1,000,000)
    //   - Cannot be sold to vendors (they ARE the sell medium)
    //   - AllowedSlots should be left empty so they cannot be equipped
    // Default false — all existing items are unaffected.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Currency")
    bool bIsCurrency = false;

    // The integer value of one unit of this currency.
    // Used by UStashComponent::CountCurrencyInStash() and SpendCurrencyFromStash()
    // to compute total spending power across mixed currency stacks:
    //   Total = Sum(StackCount * CurrencyValue) for all currency FItemInstances in stash
    //
    // Examples:
    //   DA_Currency_Rubles  → CurrencyValue = 1,   StackMax = 1,000,000
    //   DA_Currency_Dollars → CurrencyValue = 120,  StackMax = 10,000
    //   DA_Currency_Gold    → CurrencyValue = 5000, StackMax = 1,000
    //
    // Only meaningful when bIsCurrency == true. Ignored on all other items.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item | Currency",
        meta = (ClampMin = "1", EditCondition = "bIsCurrency"))
    int32 CurrencyValue = 1;
    
    // -----------------------------------------------------------------------
    // Asset Manager Integration
    // -----------------------------------------------------------------------

    // Returns the PrimaryAssetId used by the Asset Manager to track and
    // async-load this definition. Format: "ItemDefinition:AssetName"
    // Required for TSoftObjectPtr streaming to work correctly at runtime.
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};