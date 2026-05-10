// Source/ShooterGame/HUD/Inventory/QuestObjectiveRowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/QuestTypes.h"
#include "QuestObjectiveRowWidget.generated.h"

// ============================================================================
// UQuestObjectiveRowWidget
//
// Reusable single-row widget for one FQuestObjective inside the questbook.
// Displays the objective description and live progress count.
//
// USAGE:
// UQuestbookWidget creates one of these per objective in the active quest.
// Refresh() is called whenever OnQuestProgressUpdated fires.
//
// MODULARITY NOTES:
// - This widget is deliberately dumb — it only knows about one objective row.
// - All quest state logic stays in UQuestTrackerSubsystem.
// - Future styling (color by type, icon per EQuestObjectiveType, completion
//   strikethrough) is added in the WBP_QuestObjectiveRow Blueprint child
//   by overriding OnObjectiveRefreshed_BP.
// ============================================================================
UCLASS(Abstract)
class SHOOTERGAME_API UQuestObjectiveRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // Called by the parent questbook widget to populate this row.
    // ObjectiveIndex is the index into UQuestDefinition::Objectives.
    UFUNCTION(BlueprintCallable, Category = "Quest|UI")
    void Refresh(const FQuestObjective& Objective, int32 CurrentProgress, bool bIsComplete);

    // Returns the objective index this row was last refreshed with.
    UFUNCTION(BlueprintPure, Category = "Quest|UI")
    int32 GetObjectiveIndex() const { return CachedObjectiveIndex; }

protected:
    // Override in WBP_QuestObjectiveRow to update text blocks, progress bars,
    // completion indicators, and per-type icons.
    //
    // FUTURE: Add EQuestObjectiveType parameter here when per-type visual
    // styling becomes a priority (different icons for Kill vs Collect etc).
    UFUNCTION(BlueprintImplementableEvent, Category = "Quest|UI")
    void OnObjectiveRefreshed_BP(const FText& Description, int32 Current, int32 Required, bool bComplete);

private:
    int32 CachedObjectiveIndex = INDEX_NONE;
    int32 CachedRequired = 0;
};