// Source/ShooterGame/HUD/Inventory/VendorWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/VendorTypes.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "VendorWidget.generated.h"

class AVendorNPCActor;
class UQuestTrackerSubsystem;
class UQuestDefinition;

// ============================================================================
// UVendorWidget
//
// Two-panel vendor UI: vendor stock on the left, player stash on the right.
// Also exposes the vendor's available quests for acceptance / turn-in.
//
// BINDING MODEL:
// - Initialized via InitVendor() called by AShooterHUD when the vendor
//   interaction delegate fires.
// - Stock display and quest list are populated once on open and re-filtered
//   whenever reputation changes.
// - Buy/sell validation is always server-side — this widget is view-only.
//
// MODULARITY NOTES:
// - The actual buy/sell server RPCs will be added to AVendorNPCActor in a
//   later step. This widget calls them when they exist; for now the Blueprint
//   events fire so visual flow can be tested without live transactions.
// - Quest accept / turn-in routes through UQuestTrackerSubsystem, not through
//   this widget's own state.
// - Future features (item inspect, bulk purchase, sell-all button, reputation
//   progress bar) extend the BP child without touching this C++ base.
//
// BLUEPRINT SETUP (WBP_VendorWidget):
// - Left panel: scroll box populated by OnStockRefreshed_BP.
// - Right panel: reference to WBP_StashWindow (already built in Phase 3).
// - Quest list panel: populated by OnQuestListRefreshed_BP.
// ============================================================================
UCLASS(Abstract)
class SHOOTERGAME_API UVendorWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Called by AShooterHUD immediately after creating this widget,
    // passing the vendor the player just interacted with.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void InitVendor(AVendorNPCActor* Vendor);

    // Rebuilds the stock list, filtering entries by current reputation.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void RefreshStock();

    // Rebuilds the available quest list for this vendor.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void RefreshQuestList();

    // Called when the player clicks a buy button in the Blueprint child.
    // Validates client-side preconditions then dispatches the server RPC.
    //
    // FUTURE: When AVendorNPCActor::ServerPurchaseItem() exists, call it here.
    // For now this fires the Blueprint event so UI flow can be built.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void RequestPurchase(const FVendorInventoryEntry& Entry);

    // Called when the player clicks a sell button.
    //
    // FUTURE: Route to AVendorNPCActor::ServerSellItem() when implemented.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void RequestSell(const FItemInstance& Item);

    // Called when the player clicks to accept a quest from this vendor.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void RequestAcceptQuest(UQuestDefinition* QuestDefinition);

    // Called when the player clicks to turn in a completed quest.
    UFUNCTION(BlueprintCallable, Category = "Vendor|UI")
    void RequestTurnInQuest(UQuestDefinition* QuestDefinition);

    UFUNCTION(BlueprintPure, Category = "Vendor|UI")
    AVendorNPCActor* GetCurrentVendor() const { return CurrentVendor; }

    UFUNCTION(BlueprintPure, Category = "Vendor|UI")
    EVendorRole GetCurrentVendorRole() const;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Blueprint implementable events
    // -------------------------------------------------------------------------

    // Called after RefreshStock() — populate your left panel with stock rows.
    // FilteredStock already has reputation gating applied.
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor|UI")
    void OnStockRefreshed_BP(const TArray<FVendorInventoryEntry>& FilteredStock);

    // Called after RefreshQuestList() — populate your quest panel.
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor|UI")
    void OnQuestListRefreshed_BP(const TArray<UQuestDefinition*>& AvailableQuests);

    // Called after a purchase request is dispatched.
    // Use this to play feedback animations, show pending state, etc.
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor|UI")
    void OnPurchaseRequested_BP(const FVendorInventoryEntry& Entry);

    // Called after a sell request is dispatched.
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor|UI")
    void OnSellRequested_BP(const FItemInstance& Item);

    // Called when vendor reputation changes and stock may need re-filtering.
    // FUTURE: Hook into a reputation-changed delegate when it exists.
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor|UI")
    void OnReputationUpdated_BP(float NewReputationLevel);

private:
    UPROPERTY()
    TObjectPtr<AVendorNPCActor> CurrentVendor;

    UPROPERTY()
    TObjectPtr<UQuestTrackerSubsystem> QuestTrackerSubsystem;

    // Returns the player's current reputation for the active vendor role.
    float GetCurrentReputation() const;
};