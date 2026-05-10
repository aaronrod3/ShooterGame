// Source/ShooterGame/Quest/QuestDefinition.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "QuestDefinition.generated.h"

// ============================================================================
// UQuestDefinition
//
// Static authoring asset for a quest/contract.
//
// IMPORTANT ARCHITECTURE RULE:
// - This asset contains DESIGN-TIME quest data only.
// - It must never store per-player runtime state such as live progress,
//   accepted/completed flags, timestamps, or replicated counters.
// - Player-specific quest state belongs in FQuestState inside QuestTypes.h
//   and is owned by UQuestTrackerSubsystem / save game data.
//
// WHY THIS STAYS MODULAR:
// - Vendors, mission systems, questbook UI, and future dialogue systems
//   can all point at the same UQuestDefinition asset.
// - Expanding quest features later should usually mean adding new optional
//   data fields here, not rewriting the tracking architecture.
//
// FUTURE EXTENSION IDEAS:
// - Prerequisite quests / unlock chains
// - Minimum vendor reputation requirements
// - Repeatable / daily contract flags
// - Time-limited contract windows
// - Cinematic briefing assets / voice-over references
// - Objective-specific map marker styling
// - Difficulty tiers and reward scaling
// ============================================================================
UCLASS(BlueprintType)
class SHOOTERGAME_API UQuestDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UQuestDefinition();

    // ---------------------------------------------------------------------
    // Core identity
    // ---------------------------------------------------------------------

    // Player-facing title shown in vendor UI, questbook, and notifications.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FText Title;

    // Full quest description / briefing text shown in the quest details panel.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FText Description;

    // Which vendor owns/issues this quest line.
    // Used by the vendor UI, reputation rewards, and turn-in routing.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    EVendorRole IssuingVendor = EVendorRole::Quartermaster;

    // ---------------------------------------------------------------------
    // Objective data
    // ---------------------------------------------------------------------

    // Ordered objective list for this quest.
    // Runtime progress is stored separately in FQuestState::ObjectiveProgress.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    TArray<FQuestObjective> Objectives;

    // ---------------------------------------------------------------------
    // Reward data
    // ---------------------------------------------------------------------

    // Currency reward granted on successful completion and turn-in.
    // Future economy layers can reinterpret this as credits, scrip, etc.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Rewards",
        meta = (ClampMin = "0.0"))
    float CurrencyReward = 0.0f;

    // Reputation granted to the issuing vendor when the quest is completed.
    // Future versions can split this into per-vendor/faction rewards if needed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Rewards",
        meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float ReputationReward = 0.0f;

    // Optional item rewards granted on turn-in.
    // These are static item instances authored for reward payout.
    //
    // FUTURE NOTE:
    // If reward generation becomes more dynamic later (rarity rolls, vendor
    // tables, weighted rewards), this array can coexist with a separate
    // reward-definition struct instead of being replaced.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Rewards")
    TArray<FItemInstance> ItemRewards;

    // Arbitrary unlock token fired when the quest is completed.
    // Used later to unlock downstream quests, vendor stock tiers, map access,
    // or story-state flags without hardcoding those dependencies here.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Rewards")
    FName UnlockFlag = NAME_None;

    // ---------------------------------------------------------------------
    // Optional future-facing metadata
    // ---------------------------------------------------------------------

    // If true, this quest can be offered again after completion.
    // Not used in Phase 4 logic yet, but added now because it is low-cost,
    // high-value metadata for future contract systems.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Advanced")
    bool bIsRepeatable = false;

    // If true, turning in the quest should consume the target item(s) for
    // Deliver objectives. Collect objectives should usually leave items intact.
    //
    // This supports your design rule:
    // - Quest items are NOT consumed on turn-in unless the quest explicitly
    //   requires physical delivery.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Advanced")
    bool bConsumesItemsOnTurnIn = false;

    // ---------------------------------------------------------------------
    // Validation helpers
    // ---------------------------------------------------------------------

    // Lightweight helper for UI and runtime systems.
    UFUNCTION(BlueprintPure, Category = "Quest")
    bool HasValidObjectives() const;

    // Returns true if this definition contains at least one reward type.
    UFUNCTION(BlueprintPure, Category = "Quest")
    bool HasAnyReward() const;
};