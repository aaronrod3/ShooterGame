// Source/ShooterGame/HUD/Inventory/QuestbookWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "QuestbookWidget.generated.h"

class UQuestTrackerSubsystem;
class UQuestObjectiveRowWidget;
class UPanelWidget;

// ============================================================================
// UQuestbookWidget
//
// Persistent questbook UI. Shows Active / Available / Completed quest lists
// with per-objective progress on the selected quest.
//
// BINDING MODEL:
// - Binds to UQuestTrackerSubsystem delegates on NativeConstruct.
// - Unbinds on NativeDestruct.
// - Never polls the subsystem — all refreshes are event-driven.
//
// USAGE:
// - In lobby: opened from lobby HUD, full questbook experience.
// - In mission: opened from map overlay, read-only active quest view.
//
// MODULARITY NOTES:
// - Tab switching (Active/Available/Completed) is handled in Blueprint via
//   OnTabChanged_BP. The C++ layer only manages data binding and refresh logic.
// - Quest selection state is tracked here so child rows stay stateless.
// - Future features (quest abandon, quest details panel, objective map pin
//   toggle) attach here without rewriting the subsystem binding logic.
//
// BLUEPRINT SETUP (WBP_QuestbookWidget):
// - Create three panel containers (one per tab) and bind them as variables.
// - Call SetActiveTab() from tab button OnClicked events.
// - Override OnQuestListRefreshed_BP to populate the panels with quest rows.
// ============================================================================
UCLASS(Abstract)
class SHOOTERGAME_API UQuestbookWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Tab control
    // -------------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "Quest|UI")
    void SetActiveTab(EQuestbookTab Tab);

    UFUNCTION(BlueprintPure, Category = "Quest|UI")
    EQuestbookTab GetActiveTab() const { return CurrentTab; }

    // -------------------------------------------------------------------------
    // Manual refresh entry point
    // -------------------------------------------------------------------------

    // Called on open and after any quest status change.
    UFUNCTION(BlueprintCallable, Category = "Quest|UI")
    void RefreshQuestList();

    // Called when a player selects a quest row to view its objectives.
    UFUNCTION(BlueprintCallable, Category = "Quest|UI")
    void SelectQuest(UQuestDefinition* QuestDefinition);

    // Refreshes the objective list for the currently selected quest.
    UFUNCTION(BlueprintCallable, Category = "Quest|UI")
    void RefreshObjectiveList();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // -------------------------------------------------------------------------
    // Blueprint implementable events
    // Implement visuals in WBP_QuestbookWidget — C++ drives the data.
    // -------------------------------------------------------------------------

    // Called when the quest list for the current tab should be rebuilt.
    // QuestStates is the filtered list for the current tab.
    // Override to populate your scroll box / list view with quest rows.
    UFUNCTION(BlueprintImplementableEvent, Category = "Quest|UI")
    void OnQuestListRefreshed_BP(const TArray<FQuestState>& QuestStates);

    // Called when the selected quest's objectives should be rebuilt.
    // Override to populate objective rows using UQuestObjectiveRowWidget.
    UFUNCTION(BlueprintImplementableEvent, Category = "Quest|UI")
    void OnObjectiveListRefreshed_BP(const TArray<FQuestObjective>& Objectives, const TArray<int32>& Progress);

    // Called when a tab changes so Blueprint can animate tab highlighting.
    UFUNCTION(BlueprintImplementableEvent, Category = "Quest|UI")
    void OnTabChanged_BP(EQuestbookTab NewTab);

    // Called when any objective progress changes for an active quest.
    // ObjectiveIndex identifies which row to update.
    UFUNCTION(BlueprintImplementableEvent, Category = "Quest|UI")
    void OnObjectiveProgressUpdated_BP(int32 ObjectiveIndex, int32 NewProgress, int32 Required);

private:
    // -------------------------------------------------------------------------
    // Subsystem binding
    // -------------------------------------------------------------------------

    UPROPERTY()
    TObjectPtr<UQuestTrackerSubsystem> QuestTrackerSubsystem;

    UFUNCTION()
    void HandleQuestProgressUpdated(const FQuestState& UpdatedState, int32 ObjectiveIndex);

    UFUNCTION()
    void HandleQuestStatusChanged(const FQuestState& UpdatedState);

    // -------------------------------------------------------------------------
    // Internal state
    // -------------------------------------------------------------------------

    EQuestbookTab CurrentTab = EQuestbookTab::Active;

    UPROPERTY()
    TObjectPtr<UQuestDefinition> SelectedQuest;
};