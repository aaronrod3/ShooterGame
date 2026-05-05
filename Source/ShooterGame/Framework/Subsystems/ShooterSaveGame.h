// Source/ShooterGame/Framework/Subsystems/ShooterSaveGame.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShooterGame/Types/LoadoutTypes.h"
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
    static constexpr int32 CurrentSaveVersion = 1;

    // Version stored in the file. Checked on load against CurrentSaveVersion.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    int32 SaveVersion = CurrentSaveVersion;

    // -----------------------------------------------------------------------
    // Saved Data
    // -----------------------------------------------------------------------

    // The player's saved loadout preset — restored into PlayerState on login.
    // One loadout for now. Extend to TArray<FLoadoutData> for multiple presets.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    FLoadoutData SavedLoadout;

    // The player's saved appearance choices — restored into PlayerState on login.
    UPROPERTY(VisibleAnywhere, Category = "Save")
    FCharacterAppearance SavedAppearance;

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