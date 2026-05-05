// Source/ShooterGame/Components/LoadoutComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "LoadoutComponent.generated.h"

// Forward declare to avoid a hard include — ItemDefinition is only needed
// in .cpp where we actually read its properties
class UItemDefinition;

// ============================================================================
// Delegates
//
// Broadcast on all clients via OnRep so any system (CombatComponent, HUD,
// AppearanceComponent) can react to loadout changes without polling.
//
// --- HOW TO BIND ---
// In AShooterGameCharacter::PostInitializeComponents():
//   LoadoutComp->OnLoadoutChanged.AddUObject(Combat, &UCombatComponent::OnLoadoutUpdated);
//   LoadoutComp->OnAppearanceChanged.AddUObject(this, &AShooterGameCharacter::OnAppearanceUpdated);
// ============================================================================
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadoutChanged, const FLoadoutData&, NewLoadout);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAppearanceChanged, const FCharacterAppearance&, NewAppearance);

// ============================================================================
// ULoadoutComponent
//
// Replicated component that owns the player's configured loadout and
// appearance data at runtime. Lives on AShooterGameCharacter.
//
// RESPONSIBILITIES:
//   - Store and replicate FLoadoutData and FCharacterAppearance
//   - Validate slot assignments via CanEquipInSlot()
//   - Broadcast delegates on change so other systems can react
//   - Accept server RPCs from lobby UI to update loadout
//
// NOT RESPONSIBLE FOR:
//   - Spawning weapon actors          (UCombatComponent, Step 6)
//   - Attaching unequipped visuals    (UCombatComponent, Step 6)
//   - Saving to disk                  (UShooterSaveGameSubsystem, Step 5B)
//   - Persisting across level travel  (AShooterPlayerState, Step 5A)
//
// --- HOW TO EXTEND ---
// To add new validated loadout logic (e.g. class restrictions, unlock checks):
//   Add a new helper function following the CanEquipInSlot() pattern.
// To add new replicated state (e.g. active perk slot):
//   Add a UPROPERTY(ReplicatedUsing=OnRep_X) and register in GetLifetimeReplicatedProps.
// ============================================================================
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API ULoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    ULoadoutComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // -----------------------------------------------------------------------
    // Delegates — bind to these to react to loadout/appearance changes
    // -----------------------------------------------------------------------

    // Fired on all clients when LoadoutData changes.
    // UCombatComponent binds this to update unequipped weapon visuals.
    UPROPERTY(BlueprintAssignable, Category = "Loadout | Events")
    FOnLoadoutChanged OnLoadoutChanged;

    // Fired on all clients when Appearance changes.
    // Character mesh/material system binds this to swap visuals.
    UPROPERTY(BlueprintAssignable, Category = "Loadout | Events")
    FOnAppearanceChanged OnAppearanceChanged;

    // -----------------------------------------------------------------------
    // Public Interface — called by lobby UI (via server RPC) and PlayerState
    // -----------------------------------------------------------------------

    // Sets a single slot on the server. Validates the assignment first.
    // Call this from the lobby UI when the player picks an item for a slot.
    // bBroadcast: set false when bulk-loading from PlayerState to avoid
    // firing OnLoadoutChanged once per slot — call BroadcastCurrentLoadout() after.
    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void Server_SetSlot(EEquipmentSlot Slot, const FLoadoutSlot& NewSlotData);

    // Replaces the entire loadout at once (used when PlayerState pushes saved
    // data to the component on spawn). Fires OnLoadoutChanged once at the end.
    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void Server_SetFullLoadout(const FLoadoutData& NewLoadout);

    // Updates appearance data on the server. Fires OnAppearanceChanged on all clients.
    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void Server_SetAppearance(const FCharacterAppearance& NewAppearance);

    // Force-fires OnLoadoutChanged with the current data.
    // Used after bulk-loading (Server_SetFullLoadout) to notify listeners once.
    void BroadcastCurrentLoadout();

    // -----------------------------------------------------------------------
    // Validation
    // -----------------------------------------------------------------------

    // Returns true if the given item definition is allowed in the given slot.
    // Checks ItemDef->AllowedSlots — if the slot is in that list, returns true.
    // Returns false if ItemDef is null or AllowedSlots is empty.
    //
    // --- HOW TO EXTEND VALIDATION ---
    // Add additional checks here for future systems:
    //   - Player class restrictions (e.g. only Medic can equip medkit)
    //   - Unlock level checks (e.g. ItemDef->UnlockLevel <= PlayerLevel)
    //   - Unique item limits (e.g. only one launcher per team)
    UFUNCTION(BlueprintCallable, Category = "Loadout")
    bool CanEquipInSlot(UItemDefinition* ItemDef, EEquipmentSlot Slot) const;

    // -----------------------------------------------------------------------
    // Read Accessors — safe to call from any client
    // -----------------------------------------------------------------------

    // Returns the full current loadout (read-only)
    FORCEINLINE const FLoadoutData& GetLoadoutData() const { return LoadoutData; }

    // Returns a single slot's data (read-only)
    FORCEINLINE const FLoadoutSlot& GetSlot(EEquipmentSlot Slot) const { return LoadoutData.GetSlot(Slot); }

    // Returns true if the given slot has an item assigned
    FORCEINLINE bool HasItemInSlot(EEquipmentSlot Slot) const { return LoadoutData.GetSlot(Slot).IsOccupied(); }

    // Returns the current appearance data (read-only)
    FORCEINLINE const FCharacterAppearance& GetAppearance() const { return Appearance; }

protected:

    virtual void BeginPlay() override;

private:

    // -----------------------------------------------------------------------
    // Replicated State
    // -----------------------------------------------------------------------

    // The player's full configured loadout. Replicated to all clients so the
    // lobby and in-game HUD stay in sync everywhere.
    UPROPERTY(ReplicatedUsing = OnRep_LoadoutData)
    FLoadoutData LoadoutData;

    // The player's cosmetic appearance choices. Replicated so other players
    // see correct mesh/skin in lobby and in-game.
    UPROPERTY(ReplicatedUsing = OnRep_Appearance)
    FCharacterAppearance Appearance;

    // -----------------------------------------------------------------------
    // Rep Notifies
    // -----------------------------------------------------------------------

    UFUNCTION()
    void OnRep_LoadoutData();

    UFUNCTION()
    void OnRep_Appearance();

    // -----------------------------------------------------------------------
    // Internal Helpers
    // -----------------------------------------------------------------------

    // Validates that the owning actor is the server before mutating state.
    // All state mutation must be server-authoritative.
    bool IsServer() const;
};