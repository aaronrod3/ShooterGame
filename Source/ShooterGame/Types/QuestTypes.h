// Source/ShooterGame/Types/QuestTypes.h
#pragma once

#include "CoreMinimal.h"
#include "QuestTypes.generated.h"

// ============================================================================
// EQuestObjectiveType
//
// The four objective categories that FQuestObjective supports.
// Collect  — find X of item Y and extract with it in inventory
// Deliver  — bring a specific item from stash to vendor (physical hand-in)
// Kill     — eliminate X enemies of a given class or tag
// Reach    — travel to a named map location tag
// ============================================================================
UENUM(BlueprintType)
enum class EQuestObjectiveType : uint8
{
    Collect     UMETA(DisplayName = "Collect"),
    Deliver     UMETA(DisplayName = "Deliver"),
    Kill        UMETA(DisplayName = "Kill"),
    Reach       UMETA(DisplayName = "Reach")
};

// Tab selection for UQuestbookWidget.
// Defined here at global scope because UHT does not allow UENUM inside a class body.
UENUM(BlueprintType)
enum class EQuestbookTab : uint8
{
    Active      UMETA(DisplayName = "Active"),
    Available   UMETA(DisplayName = "Available"),
    Completed   UMETA(DisplayName = "Completed")
};


// ============================================================================
// EQuestStatus
//
// Lifecycle state of a quest instance in the player's questbook.
// Locked       — prerequisites not met; not visible to the player
// Available    — offered by vendor; player has not accepted yet
// Active       — player accepted; objectives being tracked
// PendingTurnIn — all objectives met; player must return to vendor
// Completed    — turned in; rewards collected; permanent record
// ============================================================================
UENUM(BlueprintType)
enum class EQuestStatus : uint8
{
    Locked          UMETA(DisplayName = "Locked"),
    Available       UMETA(DisplayName = "Available"),
    Active          UMETA(DisplayName = "Active"),
    PendingTurnIn   UMETA(DisplayName = "Pending Turn-In"),
    Completed       UMETA(DisplayName = "Completed")
};


// ============================================================================
// EVendorRole
//
// The five vendor archetypes in the lobby.
// Used on FVendorReputationEntry and AVendorNPCActor to identify
// which vendor's reputation and stock are being referenced.
// ============================================================================
UENUM(BlueprintType)
enum class EVendorRole : uint8
{
    Quartermaster   UMETA(DisplayName = "Quartermaster"),
    Gunsmith        UMETA(DisplayName = "Gunsmith"),
    Medic           UMETA(DisplayName = "Medic"),
    IntelBroker     UMETA(DisplayName = "Intel Broker"),
    Armorer         UMETA(DisplayName = "Armorer")
};


// ============================================================================
// FQuestObjective
//
// One objective entry inside a UQuestDefinition.
// CurrentCount is a runtime field — it is NOT saved on the data asset.
// It is copied into FQuestState::ObjectiveProgress (parallel int array)
// so the data asset remains pristine and shareable across players.
//
// TargetItem       — used by Collect and Deliver objectives.
//                    Soft pointer; resolved at runtime against stash/inventory.
// TargetEnemyClass — used by Kill objectives.
// TargetLocationTag — used by Reach objectives.
//                    Must match a tag registered in ShooterGameplayTags.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FQuestObjective
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    EQuestObjectiveType ObjectiveType = EQuestObjectiveType::Collect;

    // Human-readable label shown in the questbook and map overlay
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    FText Description;

    // Item to collect or deliver — leave null for Kill/Reach objectives
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    TSoftObjectPtr<class UItemDefinition> TargetItem;

    // Enemy class to kill — leave null for non-Kill objectives
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    TSoftClassPtr<AActor> TargetEnemyClass;

    // Gameplay tag identifying the map location — used by Reach objectives
    // Must match a registered tag in ShooterGameplayTags.h
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective")
    FName TargetLocationTag = NAME_None;

    // How many units/kills/visits are required to complete this objective
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Objective",
        meta = (ClampMin = "1"))
    int32 RequiredCount = 1;
};


// ============================================================================
// FQuestState
//
// The runtime record of one quest in a player's questbook.
// Held in UQuestTrackerSubsystem and persisted to UShooterSaveGame.
//
// ObjectiveProgress is a parallel int array matching the index order of
// UQuestDefinition::Objectives. Never access by name — always by index.
//
// bStashCountedOnActivation — set true the first time AutoCountStash() runs
// for this quest so stash counting never runs twice for the same activation.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FQuestState
{
    GENERATED_BODY()

    // Soft pointer to the data asset so this struct is safe to serialize
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    TSoftObjectPtr<class UQuestDefinition> Definition;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    EQuestStatus Status = EQuestStatus::Available;

    // Parallel array to UQuestDefinition::Objectives — one int per objective
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    TArray<int32> ObjectiveProgress;

    // Guards against running AutoCountStash more than once per activation
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest")
    bool bStashCountedOnActivation = false;

    // Returns true if all objectives are at or above their RequiredCount
    // Requires a resolved (loaded) Definition pointer — check IsNull() first
    bool AreAllObjectivesComplete(const class UQuestDefinition* ResolvedDef) const;
    
    // Equality by Definition pointer — two FQuestStates are the same quest
    // if they point at the same UQuestDefinition asset.
    bool operator==(const FQuestState& Other) const
    {
        return Definition == Other.Definition;
    }
};


// ============================================================================
// FVendorReputationEntry
//
// One vendor's reputation record for a single player.
// Held in TArray<FVendorReputationEntry> in UQuestTrackerSubsystem.
// Persisted to UShooterSaveGame alongside quest state.
// ============================================================================
USTRUCT(BlueprintType)
struct SHOOTERGAME_API FVendorReputationEntry
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor")
    EVendorRole VendorRole = EVendorRole::Quartermaster;

    // 0.0 = neutral, 100.0 = max trust. Increases via quests and relevant item sales.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float ReputationLevel = 0.0f;
};