// Source/ShooterGame/Interaction/QuestGiverInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "QuestGiverInterface.generated.h"

UINTERFACE(BlueprintType)
class SHOOTERGAME_API UQuestGiverInterface : public UInterface
{
    GENERATED_BODY()
};

// ============================================================================
// IQuestGiverInterface
//
// Modular contract for anything in the world that can offer, activate, or
// accept turn-in of quests.
//
// WHY THIS EXISTS AS A SEPARATE INTERFACE:
// - AVendorNPCActor is the first quest source in Phase 4.
// - Future sources (field radios, mission terminals, extraction officers,
//   squad leaders, scripted world actors) can implement this interface
//   without inheriting from AVendorNPCActor or any vendor-specific class.
// - Keeping quest-giving logic behind an interface prevents the quest system
//   from being tightly coupled to any one actor type.
//
// UHT NOTE:
// UFUNCTION parameters and return values must use raw pointers (UFoo*),
// not TObjectPtr<UFoo>. TObjectPtr is only valid on UPROPERTY fields.
//
// FUTURE EXTENSION IDEAS:
// - Add GetQuestGiverDisplayName() for UI label
// - Add GetQuestGiverIcon() for map/questbook icon
// - Add OnQuestAccepted / OnQuestTurnedIn delegates for audio/animation hooks
// - Add GetPrerequisiteFlags() to drive unlock chain evaluation
// ============================================================================
class SHOOTERGAME_API IQuestGiverInterface
{
    GENERATED_BODY()

public:
    // Returns all quest definitions this giver can currently offer.
    // Implementations should filter by player quest status, prerequisites,
    // and reputation before returning — the returned list is what the UI shows.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest")
    TArray<UQuestDefinition*> GetAvailableQuests() const;

    // Returns true if this actor currently has at least one quest to offer.
    // Used for UI prompt visibility and map marker toggling.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest")
    bool OffersQuests() const;

    // Returns true if this giver can accept a turn-in for the given quest.
    // Implementations can gate on location, state, or cooldowns.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest")
    bool CanTurnInQuest(UQuestDefinition* QuestDefinition) const;

    // Called when the player attempts to turn in a completed quest.
    // Returns true if the turn-in was accepted.
    //
    // IMPORTANT:
    // This function should be a thin dispatcher only.
    // Actual objective validation, reward payout, rep mutation, and save
    // persistence must be delegated to UQuestTrackerSubsystem server-side.
    // Never commit quest state changes directly inside this implementation.
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest")
    bool TurnInQuest(APlayerController* InteractingPlayer, UQuestDefinition* QuestDefinition);
};