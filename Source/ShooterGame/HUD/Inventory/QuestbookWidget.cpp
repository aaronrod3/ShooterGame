// Source/ShooterGame/HUD/Inventory/QuestbookWidget.cpp
#include "ShooterGame/HUD/Inventory/QuestbookWidget.h"

#include "ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h"
#include "ShooterGame/Quest/QuestDefinition.h"

void UQuestbookWidget::NativeConstruct()
{
    Super::NativeConstruct();

    QuestTrackerSubsystem = GetGameInstance()->GetSubsystem<UQuestTrackerSubsystem>();

    if (QuestTrackerSubsystem)
    {
        QuestTrackerSubsystem->OnQuestProgressUpdated.AddDynamic(
            this, &UQuestbookWidget::HandleQuestProgressUpdated);

        QuestTrackerSubsystem->OnQuestStatusChanged.AddDynamic(
            this, &UQuestbookWidget::HandleQuestStatusChanged);
    }

    RefreshQuestList();
}

void UQuestbookWidget::NativeDestruct()
{
    if (QuestTrackerSubsystem)
    {
        QuestTrackerSubsystem->OnQuestProgressUpdated.RemoveDynamic(
            this, &UQuestbookWidget::HandleQuestProgressUpdated);

        QuestTrackerSubsystem->OnQuestStatusChanged.RemoveDynamic(
            this, &UQuestbookWidget::HandleQuestStatusChanged);
    }

    Super::NativeDestruct();
}

void UQuestbookWidget::SetActiveTab(EQuestbookTab Tab)
{
    CurrentTab = Tab;
    SelectedQuest = nullptr;
    OnTabChanged_BP(Tab);
    RefreshQuestList();
}

void UQuestbookWidget::RefreshQuestList()
{
    if (!QuestTrackerSubsystem)
    {
        return;
    }

    TArray<FQuestState> QuestsForTab;

    switch (CurrentTab)
    {
    case EQuestbookTab::Active:
        QuestsForTab = QuestTrackerSubsystem->GetActiveQuests();
        break;
    case EQuestbookTab::Available:
        QuestsForTab = QuestTrackerSubsystem->GetAvailableQuests();
        break;
    case EQuestbookTab::Completed:
        QuestsForTab = QuestTrackerSubsystem->GetCompletedQuests();
        break;
    }

    OnQuestListRefreshed_BP(QuestsForTab);
}

void UQuestbookWidget::SelectQuest(UQuestDefinition* QuestDefinition)
{
    SelectedQuest = QuestDefinition;
    RefreshObjectiveList();
}

void UQuestbookWidget::RefreshObjectiveList()
{
    if (!IsValid(SelectedQuest))
    {
        return;
    }

    const FQuestState* State = QuestTrackerSubsystem
        ? QuestTrackerSubsystem->FindQuestState(SelectedQuest)
        : nullptr;

    if (!State)
    {
        return;
    }

    OnObjectiveListRefreshed_BP(SelectedQuest->Objectives, State->ObjectiveProgress);
}

void UQuestbookWidget::HandleQuestProgressUpdated(const FQuestState& UpdatedState, int32 ObjectiveIndex)
{
    // Only update the objective list if the progress event is for the
    // currently selected quest — avoids unnecessary full rebuilds.
    if (UpdatedState.Definition.Get() == SelectedQuest
        && UpdatedState.ObjectiveProgress.IsValidIndex(ObjectiveIndex))
    {
        const FQuestObjective& Objective = SelectedQuest->Objectives[ObjectiveIndex];
        OnObjectiveProgressUpdated_BP(
            ObjectiveIndex,
            UpdatedState.ObjectiveProgress[ObjectiveIndex],
            Objective.RequiredCount
        );
    }
}

void UQuestbookWidget::HandleQuestStatusChanged(const FQuestState& /*UpdatedState*/)
{
    // Any status change (accepted, completed, pending turn-in) warrants
    // a full list rebuild since the quest may have moved between tabs.
    RefreshQuestList();
}