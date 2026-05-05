// Source/ShooterGame/Framework/PlayerState/ShooterPlayerState.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ShooterPlayerState.generated.h"


class ULoadoutComponent;


// ============================================================================
// AShooterPlayerState
//
// Persists the player's loadout and appearance selections across level travel,
// respawns, and lobby/game transitions.
//
// DATA FLOW:
//
//   [Disk / UShooterSaveGameSubsystem]
//           │  LoadSaveData() on login
//           ▼
//   [AShooterPlayerState]  ← source of truth for the session
//           │  PushLoadoutToCharacter() on possess
//           ▼
//   [ULoadoutComponent on AShooterGameCharacter]  ← runtime state
//           │  OnLoadoutChanged delegate
//           ▼
//   [UCombatComponent / HUD / Appearance]  ← react to changes
//
//   When the player edits loadout in lobby:
//   [Lobby UI] → Server_UpdateLoadoutSlot() → [PlayerState] → [LoadoutComponent]
//                                                           → [SaveSubsystem writes disk]
//
// WHY PLAYERSTATE AND NOT JUST LOADOUTCOMPONENT:
//   ULoadoutComponent lives on ACharacter which is destroyed on death/respawn.
//   APlayerState survives the full session and level travel (bUseSeamlessTravel).
//   This separation means loadout data is never lost on death.
//
// --- HOW TO EXTEND ---
// Add new replicated session data here (player name, score, team assignment).
// For new persistent data beyond loadout, also extend UShooterSaveGame (Step 5B).
// ============================================================================
UCLASS()
class SHOOTERGAME_API AShooterPlayerState : public APlayerState
{
    GENERATED_BODY()

public:

    AShooterPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // -----------------------------------------------------------------------
    // Loadout Interface — called by lobby UI and SaveSubsystem
    // -----------------------------------------------------------------------

    // Sets a single slot in the saved loadout and pushes the change to the
    // character's LoadoutComponent if one is currently possessed.
    // Also notifies the SaveSubsystem to write to disk.
    // Call this from the lobby UI when the player picks an item.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loadout")
    void Server_UpdateLoadoutSlot(EEquipmentSlot Slot, const FLoadoutSlot& NewSlotData);

    // Replaces the entire saved loadout at once.
    // Called by UShooterSaveGameSubsystem on login to restore saved data.
    // bPushToCharacter: false during initial load before a character exists.
    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void SetFullLoadout(const FLoadoutData& NewLoadout, bool bPushToCharacter = false);

    // Updates the saved appearance and pushes to character if possessed.
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Loadout")
    void Server_UpdateAppearance(const FCharacterAppearance& NewAppearance);

    // -----------------------------------------------------------------------
    // Character Possession Bridge
    // -----------------------------------------------------------------------

    // Called by the PlayerController (or GameMode) when this player possesses
    // a new character. Pushes the current saved loadout and appearance into
    // the character's LoadoutComponent so it starts with correct data.
    //
    // Call this from AShooterGameCharacter::PossessedBy() or from
    // AShooterGameMode::RestartPlayer() after possession is complete.
    UFUNCTION(BlueprintCallable, Category = "Loadout")
    void PushLoadoutToCharacter(ACharacter* NewCharacter);

    // -----------------------------------------------------------------------
    // Read Accessors
    // -----------------------------------------------------------------------

    FORCEINLINE const FLoadoutData&        GetSavedLoadout()    const { return SavedLoadout; }
    FORCEINLINE const FCharacterAppearance& GetSavedAppearance() const { return SavedAppearance; }

protected:

    // APlayerState::CopyProperties is called when the engine re-creates a
    // PlayerState after seamless travel. Override to carry loadout data across.
    virtual void CopyProperties(APlayerState* PlayerState) override;

private:

    // -----------------------------------------------------------------------
    // Replicated State
    // -----------------------------------------------------------------------

    // The player's saved loadout configuration for this session.
    // Replicated so the lobby can show other players' loadout choices.
    // Initialized empty — populated by UShooterSaveGameSubsystem on login.
    UPROPERTY(ReplicatedUsing = OnRep_SavedLoadout)
    FLoadoutData SavedLoadout;

    // The player's saved appearance for this session.
    UPROPERTY(ReplicatedUsing = OnRep_SavedAppearance)
    FCharacterAppearance SavedAppearance;

    // -----------------------------------------------------------------------
    // Rep Notifies
    // -----------------------------------------------------------------------

    UFUNCTION()
    void OnRep_SavedLoadout();

    UFUNCTION()
    void OnRep_SavedAppearance();

    // -----------------------------------------------------------------------
    // Internal Helpers
    // -----------------------------------------------------------------------

    // Retrieves the LoadoutComponent from the currently possessed character.
    // Returns nullptr if no character is possessed or component is missing.
    ULoadoutComponent* GetOwnedLoadoutComponent() const;

    // Notifies the SaveSubsystem that loadout data has changed and should
    // be written to disk. No-op if subsystem is not available.
    void NotifySaveSubsystem();
};