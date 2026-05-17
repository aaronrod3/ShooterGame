// Source/ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "QuestTrackerSubsystem.generated.h"

class UQuestDefinition;
class UStashComponent;

// ============================================================================
// Delegates
// ============================================================================

// Broadcast whenever any quest state or objective progress changes.
// UI widgets (questbook, HUD overlays, map markers) bind here and refresh
// themselves without polling this subsystem every frame.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnQuestProgressUpdated,
    const FQuestState&, UpdatedQuestState,
    int32, ObjectiveIndex
);

// Broadcast when a quest changes status (accepted, completed, etc).
// Separate from progress updates so widgets can handle state transitions
// distinctly from incremental objective ticks.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnQuestStatusChanged,
    const FQuestState&, UpdatedQuestState
);

// ============================================================================
// UQuestTrackerSubsystem
//
// Persistent per-player questbook and vendor reputation store.
// Lives on UGameInstance so it survives level transitions between lobby
// and mission maps without data loss.
//
// AUTHORITY MODEL:
// All state mutation functions are guarded by HasAuthority() on the outer world.
// The client UI reads from this subsystem as a view — it never writes directly.
// When moving to a fully networked session, these mutations should be driven
// by server-side RPCs that call into this subsystem on the server only.
//
// MODULARITY NOTES:
// - Add new objective types in EQuestObjectiveType and handle them in
//   RecordKillObjectiveProgress / RecordReachObjectiveProgress — no other
//   files need to change.
// - Add new vendor roles in EVendorRole and a corresponding reputation entry —
//   the TArray<FVendorReputationEntry> scales automatically.
// - Save/load integration is intentionally deferred to a later step so this
//   subsystem stays readable and testable on its own first.
//
// FUTURE EXTENSION IDEAS:
// - Daily/repeatable quest refresh timer
// - Faction reputation (different from vendor rep)
// - Quest prerequisite chain evaluator
// - Quest expiry / time-limited contracts
// - Mission-side objective broadcast receiver
// ============================================================================
UCLASS()
class SHOOTERGAME_API UQuestTrackerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // -------------------------------------------------------------------------
    // Delegates
    // -------------------------------------------------------------------------

    // Bind here to react to any objective progress change.
    // Questbook widget, HUD counters, and map overlays all bind to this.
    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestProgressUpdated OnQuestProgressUpdated;

    // Bind here to react to quest status transitions (accepted, completed, etc).
    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FOnQuestStatusChanged OnQuestStatusChanged;

    // -------------------------------------------------------------------------
    // Quest lifecycle
    // -------------------------------------------------------------------------

    // Makes a quest available to the player. Called when a vendor is unlocked
    // or when a prerequisite quest is completed.
    // Does nothing if the quest is already tracked in any state.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void MakeQuestAvailable(UQuestDefinition* QuestDefinition);

    // Accepts a quest for active tracking. Initialises objective progress,
    // then runs AutoCountStash to pre-fill counts from existing stash items.
    // Only valid if the quest is currently in Available state.
    //UFUNCTION(BlueprintCallable, Category = "Quest")
    //void ActivateQuest(UQuestDefinition* QuestDefinition, UStashComponent* PlayerStash);

    // Records incremental progress on a Collect or Kill objective.
    // ObjectiveIndex is the index into QuestDefinition::Objectives.
    // Delta is typically +1 per item collected or enemy killed.
    // Automatically transitions to PendingTurnIn when all objectives complete.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void RecordObjectiveProgress(UQuestDefinition* QuestDefinition, int32 ObjectiveIndex, int32 Delta = 1);

    // Marks a quest complete, grants reputation, and fires OnQuestStatusChanged.
    // Does NOT grant currency or item rewards — those are handled separately
    // by the economy/inventory layer when it calls this after validating turn-in.
    // Only valid if the quest is in PendingTurnIn state.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void CompleteQuest(UQuestDefinition* QuestDefinition);
    

    // -------------------------------------------------------------------------
    // Reputation
    // -------------------------------------------------------------------------

    // Returns the player's current reputation level for the given vendor role.
    // Returns 0.0 if no reputation entry exists yet.
    UFUNCTION(BlueprintPure, Category = "Quest")
    float GetReputationFor(EVendorRole VendorRole) const;

    // Adds reputation to the given vendor. Clamped to [0, 100].
    // Called by CompleteQuest internally, and externally by sell/buy systems
    // when selling relevant items to a vendor.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void AddReputation(EVendorRole VendorRole, float Amount);

    // -------------------------------------------------------------------------
    // Read access for UI and external systems
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Quest")
    const TArray<FQuestState>& GetActiveQuests() const { return ActiveQuests; }

    UFUNCTION(BlueprintPure, Category = "Quest")
    const TArray<FQuestState>& GetCompletedQuests() const { return CompletedQuests; }

    UFUNCTION(BlueprintPure, Category = "Quest")
    const TArray<FQuestState>& GetAvailableQuests() const { return AvailableQuests; }

    UFUNCTION(BlueprintPure, Category = "Quest")
    const TArray<FVendorReputationEntry>& GetAllReputations() const { return VendorReputations; }

    // Returns a pointer to the live FQuestState for a given definition,
    // searching all buckets. Returns nullptr if not found.
    // Useful for UI display and progress reads — do not mutate via this pointer.
    const FQuestState* FindQuestState(const UQuestDefinition* QuestDefinition) const;

    // Returns true if this quest has been completed at any point.
    UFUNCTION(BlueprintPure, Category = "Quest")
    bool IsQuestCompleted(UQuestDefinition* QuestDefinition) const;

    // Returns true if the given unlock flag has been fired by any completed quest.
    // Used by vendor stock unlock logic and quest prerequisite chains.
    UFUNCTION(BlueprintPure, Category = "Quest")
    bool IsUnlockFlagActive(FName UnlockFlag) const;

    // -------------------------------------------------------------------------
    // Save integration hooks (wired in a later step)
    // -------------------------------------------------------------------------

    // Populates this subsystem from serialized save data.
    // Called by UShooterSaveGameSubsystem after loading from disk.
    // All arrays are replaced, not merged.
    //
    // FUTURE: Called during lobby load, before any vendor or quest UI is shown.
    void LoadFromSaveData(
        const TArray<FQuestState>& SavedActive,
        const TArray<FQuestState>& SavedAvailable,
        const TArray<FQuestState>& SavedCompleted,
        const TArray<FVendorReputationEntry>& SavedReputations
    );

    // Provides current state for serialization into save data.
    // Called by UShooterSaveGameSubsystem before writing to disk.
    void GetSaveData(
        TArray<FQuestState>& OutActive,
        TArray<FQuestState>& OutAvailable,
        TArray<FQuestState>& OutCompleted,
        TArray<FVendorReputationEntry>& OutReputations
    ) const;

private:
    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------

    UPROPERTY()
    TArray<FQuestState> ActiveQuests;

    UPROPERTY()
    TArray<FQuestState> AvailableQuests;

    UPROPERTY()
    TArray<FQuestState> CompletedQuests;

    UPROPERTY()
    TArray<FVendorReputationEntry> VendorReputations;

    // Flat set of unlock flags fired by completed quests.
    // Checked by IsUnlockFlagActive() for prerequisite and stock gating.
    UPROPERTY()
    TArray<FName> ActiveUnlockFlags;

    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    // Returns a mutable pointer into the correct array for a given quest status.
    FQuestState* FindMutableQuestState(const UQuestDefinition* QuestDefinition, EQuestStatus InStatus);

    // Builds a fresh FQuestState with zeroed objective progress for the given definition.
    static FQuestState BuildInitialState(UQuestDefinition* QuestDefinition, EQuestStatus InitialStatus);

    // Checks if all objectives in the given state are complete and transitions
    // status to PendingTurnIn if so. Broadcasts OnQuestStatusChanged.
    void CheckAndTransitionIfComplete(FQuestState& QuestState);
};