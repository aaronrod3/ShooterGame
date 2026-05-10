// Source/ShooterGame/HUD/Inventory/QuestObjectiveRowWidget.cpp
#include "ShooterGame/HUD/Inventory/QuestObjectiveRowWidget.h"

void UQuestObjectiveRowWidget::Refresh(const FQuestObjective& Objective, int32 CurrentProgress, bool bIsComplete)
{
	CachedObjectiveIndex = INDEX_NONE;
	CachedRequired = Objective.RequiredCount;

	OnObjectiveRefreshed_BP(Objective.Description, CurrentProgress, Objective.RequiredCount, bIsComplete);
}