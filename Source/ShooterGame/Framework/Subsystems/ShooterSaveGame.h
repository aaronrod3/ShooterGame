// Source/ShooterGame/Framework/Subsystems/ShooterSaveGame.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "ShooterGame/Types/VendorTypes.h"
#include "ShooterSaveGame.generated.h"

// ============================================================================
// UShooterSaveGame
//
// The serialized save file written to disk via UGameplayStatics::SaveGameToSlot.
// In PIE this writes to: [ProjectRoot]/Saved/SaveGames/ShooterGame_Loadout.sav
//
// VERSIONING:
// SaveVersion lets us detect stale save files and migrate or discard them
// safely. Bump SaveVersion any time you remove or reorder fields in
// FLoadoutSlot, FLoadoutData, or FCharacterAppearance — NOT when adding
// new fields with default values (those are backward compatible).
//
// FUTURE PLATFORM SAVE:
// This class only handles the data shape. The actual read/write transport
// is handled by UShooterSaveGameSubsystem. To swap in EOS cloud saves,
// replace SaveGameToSlot/LoadGameFromSlot calls in the subsystem only —
// this class stays identical.
//
// --- HOW TO EXTEND ---
// Add new UPROPERTY fields freely — always provide default values.
// Never remove or reorder existing fields without bumping SaveVersion
// and adding migration logic in UShooterSaveGameSubsystem::LoadLoadout().
// Future fields to consider:
//   - TArray<FLoadoutData> SavedLoadoutPresets  (multiple named presets)
//   - FString LastPlayedCharacterClass          (restore class selection)
//   - int32 TotalMissionsCompleted              (progression tracking)
// ============================================================================
UCLASS()
class SHOOTERGAME_API UShooterSaveGame : public USaveGame
{
    GENERATED_BODY()

public:

    UShooterSaveGame();

    // -----------------------------------------------------------------------
    // Save Versioning
    // -----------------------------------------------------------------------

    // Current expected version. If a loaded file has a different version,
    // the subsystem will discard it and start fresh rather than corrupt data.
    // Bump this integer any time field layout changes in a breaking way.
    static constexpr int32 CurrentSaveVersion = 3;

    // Version stored in the file. Checked on load against CurrentSaveVersion.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    int32 SaveVersion = CurrentSaveVersion;

    // -----------------------------------------------------------------------
    // Saved Data
    // -----------------------------------------------------------------------

    // Legacy lobby loadout data already used by the existing loadout pipeline.
    // Kept intact for backward compatibility with the current character/lobby flow.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    FLoadoutData SavedLoadout;

    // The player's saved appearance choices — restored into PlayerState on login.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    FCharacterAppearance SavedAppearance;

    // Persistent personal stash contents. This is the player's safe inventory
    // layer in the lobby and is never lost on death.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    TArray<FItemInstance> SavedStashItems;

    // Persistent carried/equipped state snapshot. This is saved exactly as-is
    // after extraction and restored until the player manually reorganizes gear.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    FEquippedStateSnapshot SavedEquippedState;

    // Three named loadout presets that reference items by InstanceID.
    // Missing items are flagged during preset resolution, not erased here.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    TArray<FLoadoutPreset> SavedLoadoutPresets;

    // True after a successful extraction if the player has not yet manually
    // changed their loadout. Used by the lobby deploy prompt:
    // "You have not changed your loadout since your last extraction."
    UPROPERTY(VisibleAnywhere, Category = "Save")
    bool bHasUnreviewedExtraction = false;
    
    // Ensures the preset array exists and contains exactly three named preset slots.
    void InitializeDefaultLoadoutPresets();

    // Clears all new Phase 2 persistence fields while keeping legacy loadout
    // and appearance data intact.
    void ResetPersistentInventoryState();
    
    
    // -----------------------------------------------------------------------
    // Quest & Vendor Reputation State  (added Phase 4)
    // -----------------------------------------------------------------------

    // Active quests in progress at time of last save.
    // Restored into UQuestTrackerSubsystem::ActiveQuests on login.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    TArray<FQuestState> SavedActiveQuests;

    // Quests that have been made available but not yet accepted.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    TArray<FQuestState> SavedAvailableQuests;

    // Quests completed in prior sessions. Used for prerequisite chain
    // evaluation and unlock flag reconstruction on load.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    TArray<FQuestState> SavedCompletedQuests;

    // Per-vendor reputation levels. Initialised to 0 by
    // UQuestTrackerSubsystem if not present in save data.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    TArray<FVendorReputationEntry> SavedVendorReputations;
    

    // -----------------------------------------------------------------------
    // Save Slot Identity
    // -----------------------------------------------------------------------
    
    
    // The slot name used by UGameplayStatics::SaveGameToSlot/LoadGameFromSlot.
    // Single slot for now — extend to per-player slots by appending a player
    // ID suffix when implementing multi-profile or platform account linking.
    static const FString SaveSlotName;

    // User index — always 0 for single-profile. Update when adding local
    // multiplayer profile support.
    static constexpr int32 UserIndex = 0;
};