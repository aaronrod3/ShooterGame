// Source/ShooterGame/Framework/Subsystems/ShooterSaveGameSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ShooterSaveGameSubsystem.generated.h"

class UShooterSaveGame;
class AShooterPlayerState;

// ============================================================================
// UShooterSaveGameSubsystem
//
// GameInstance subsystem that owns all save/load operations for the game.
// Lives for the entire application session — survives level travel.
//
// RESPONSIBILITIES:
//   - Load save data from disk on startup (Initialize)
//   - Write save data to disk when loadout changes (SaveLoadout)
//   - Provide the in-memory cached save object to PlayerState on login
//   - Expose debug console commands in non-shipping builds
//
// NOT RESPONSIBLE FOR:
//   - Replicating data to clients     (AShooterPlayerState handles that)
//   - Spawning actors                 (UCombatComponent handles that)
//   - Platform cloud saves            (future: swap transport here only)
//
// FUTURE PLATFORM SAVE SWAP:
// When adding EOS or Steam cloud saves, subclass this or add a virtual
// SaveToCloud() / LoadFromCloud() pair. The rest of the game only calls
// SaveLoadout() and GetCachedSaveGame() — no other code needs to change.
//
// --- HOW TO EXTEND ---
// To save new data: add fields to UShooterSaveGame and call SaveLoadout()
// from wherever the data changes. No changes needed here.
// To add new debug commands: follow the pattern in Initialize() below.
// ============================================================================
UCLASS()
class SHOOTERGAME_API UShooterSaveGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // UGameInstanceSubsystem interface
    // -----------------------------------------------------------------------

    // Called once when the GameInstance starts — before any level loads.
    // Loads save data from disk and registers debug console commands.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Called when the GameInstance is shutting down.
    // Ensures any pending save is flushed to disk before exit.
    virtual void Deinitialize() override;

    // -----------------------------------------------------------------------
    // Public Interface
    // -----------------------------------------------------------------------

    // Writes the given loadout and appearance to the in-memory cache and
    // immediately saves to disk. Called by AShooterPlayerState::NotifySaveSubsystem
    // whenever the player makes a loadout change in the lobby.
    UFUNCTION(BlueprintCallable, Category = "Save")
    void SaveLoadout(const FLoadoutData& Loadout, const FCharacterAppearance& Appearance);

    // Pushes the cached save data into the given PlayerState.
    // Call this from your GameMode or PlayerController when a player logs in
    // and their PlayerState is ready to receive data.
    UFUNCTION(BlueprintCallable, Category = "Save")
    void PushSaveDataToPlayerState(AShooterPlayerState* PlayerState);

    // Returns the raw cached save object — useful for reading any save field
    // directly without going through PlayerState.
    // Returns nullptr if no save data has been loaded yet.
    UFUNCTION(BlueprintCallable, Category = "Save")
    UShooterSaveGame* GetCachedSaveGame() const { return CachedSaveGame; }

    // Returns true if a save file exists on disk for the current slot.
    UFUNCTION(BlueprintCallable, Category = "Save")
    bool DoesSaveExist() const;

private:

    // -----------------------------------------------------------------------
    // Internal Operations
    // -----------------------------------------------------------------------

    // Reads the save file from disk into CachedSaveGame.
    // If no file exists, creates a fresh UShooterSaveGame with defaults.
    // If the file version doesn't match CurrentSaveVersion, discards and resets.
    void LoadLoadout();

    // Immediately writes CachedSaveGame to disk via SaveGameToSlot.
    void WriteToDisk();

    // -----------------------------------------------------------------------
    // Debug Commands (non-shipping only)
    // -----------------------------------------------------------------------

#if !UE_BUILD_SHIPPING
    void DebugSaveLoadout();
    void DebugLoadLoadout();
    void DebugClearSaveData();
    void DebugPrintLoadout();
#endif

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    // In-memory cache of the loaded save file.
    // Always valid after Initialize() — either loaded from disk or freshly created.
    UPROPERTY()
    TObjectPtr<UShooterSaveGame> CachedSaveGame;
};