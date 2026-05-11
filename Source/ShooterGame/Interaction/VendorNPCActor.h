// Source/ShooterGame/Interaction/VendorNPCActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Interaction/Interactable.h"
#include "ShooterGame/Interaction/QuestGiverInterface.h"
#include "ShooterGame/Types/VendorTypes.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "VendorNPCActor.generated.h"

class ACharacter;
class APlayerController;
class UCapsuleComponent;
class USceneComponent;
class UStaticMeshComponent;
class UHighlightableStaticMesh;
class UQuestDefinition;
class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVendorInteracted, APlayerController*, InteractingPlayer, AVendorNPCActor*, VendorActor);

// ============================================================================
// AVendorNPCActor
//
// Lobby-facing vendor actor for shop + quest interaction.
//
// STEP 3 SCOPE:
// - Establishes the world actor foundation only
// - Implements IInteractable using the repo's existing character-driven pattern
// - Implements IQuestGiverInterface for authored quest exposure
// - Stores vendor role, stock, and offered quest assets
//
// MODULARITY NOTES:
// - Keep this actor thin; it should present data and broadcast interactions
// - Persistent quest state belongs in UQuestTrackerSubsystem
// - Economy truth and inventory mutation should stay server-side elsewhere
// - Later systems can extend this actor with animation/audio/dialogue hooks
//   without needing to rewrite the quest or vendor backend
// ============================================================================
UCLASS()
class SHOOTERGAME_API AVendorNPCActor : public AActor, public IInteractable, public IQuestGiverInterface
{
    GENERATED_BODY()

public:
    AVendorNPCActor();

protected:
    virtual void BeginPlay() override;

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor|Components")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor|Components")
    TObjectPtr<UStaticMeshComponent> VendorMesh;

    // Future interaction range checks should use this component as the
    // authoritative overlap/query shape rather than hardcoding distances.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor|Components")
    TObjectPtr<UCapsuleComponent> InteractionCapsule;

    // Mirrors the existing highlightable pattern used elsewhere in Interaction/.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vendor|Components")
    TObjectPtr<UHighlightableStaticMesh> HighlightMeshComponent;

    // -------------------------------------------------------------------------
    // Authored vendor data
    // -------------------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    EVendorRole VendorRole = EVendorRole::Quartermaster;

    // Authored stock list for this vendor archetype.
    // Runtime filtering by rep/unlocks comes later.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    TArray<FVendorInventoryEntry> VendorStock;

    // The currency item definition this vendor pays out in when the player
    // sells items. Set this in the BP_VendorNPC_* child per vendor archetype.
    //
    // Example authoring:
    //   BP_VendorNPC_Quartermaster → DA_Currency_Rubles
    //   BP_VendorNPC_BlackMarket   → DA_Currency_Dollars
    //
    // If left null, ServerSellItem will reject all sell attempts with a
    // "Vendor cannot pay out currency" error — always set this on every vendor.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor",
        meta = (DisplayThumbnail = "true"))
    TObjectPtr<UItemDefinition> PayoutCurrencyDefinition;

    // Authored quests this vendor can potentially offer.
    // Runtime systems decide which are actually available to each player.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    TArray<TObjectPtr<UQuestDefinition>> OfferedQuests;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    FText InteractPromptText;

public:
    // Broadcast when a valid character interacts and we resolve a controller.
    // HUD systems can bind here later to open vendor UI without polling actors.
    UPROPERTY(BlueprintAssignable, Category = "Vendor")
    FOnVendorInteracted OnVendorInteracted;

    // -------------------------------------------------------------------------
    // IInteractable — MUST match repo signatures exactly
    // -------------------------------------------------------------------------

    virtual void Interact_Implementation(ACharacter* InteractingCharacter) override;
    virtual bool CanInteract_Implementation(ACharacter* InteractingCharacter) const override;
    virtual FText GetInteractPromptText_Implementation(ACharacter* InteractingCharacter) const override;

    // -------------------------------------------------------------------------
    // IQuestGiverInterface
    // -------------------------------------------------------------------------

    virtual TArray<UQuestDefinition*> GetAvailableQuests_Implementation() const override;
    virtual bool OffersQuests_Implementation() const override;
    virtual bool CanTurnInQuest_Implementation(UQuestDefinition* QuestDefinition) const override;
    virtual bool TurnInQuest_Implementation(APlayerController* InteractingPlayer, UQuestDefinition* QuestDefinition) override;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "Vendor")
    EVendorRole GetVendorRole() const { return VendorRole; }

    UFUNCTION(BlueprintPure, Category = "Vendor")
    const TArray<FVendorInventoryEntry>& GetVendorStock() const { return VendorStock; }

    // Returns the currency definition this vendor pays out on player sells.
    // Always check IsValid() on the return value before use.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    UItemDefinition* GetPayoutCurrencyDefinition() const { return PayoutCurrencyDefinition; }

    // C++-only accessor. Not a UFUNCTION because TObjectPtr arrays are not
    // valid UFUNCTION return types.
    const TArray<TObjectPtr<UQuestDefinition>>& GetOfferedQuests() const { return OfferedQuests; }
};