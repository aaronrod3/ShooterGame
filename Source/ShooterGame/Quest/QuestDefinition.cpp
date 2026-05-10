// Source/ShooterGame/Quest/QuestDefinition.cpp
#include "ShooterGame/Quest/QuestDefinition.h"

UQuestDefinition::UQuestDefinition()
{
	// Keep assets discoverable and organized in the content browser.
	// This helps later when the project grows into many quest lines.
	// You can expand these tags later without affecting save architecture.
}

bool UQuestDefinition::HasValidObjectives() const
{
	if (Objectives.Num() == 0)
	{
		return false;
	}

	for (const FQuestObjective& Objective : Objectives)
	{
		if (Objective.RequiredCount <= 0)
		{
			return false;
		}

		switch (Objective.ObjectiveType)
		{
		case EQuestObjectiveType::Collect:
		case EQuestObjectiveType::Deliver:
			if (Objective.TargetItem.IsNull())
			{
				return false;
			}
			break;

		case EQuestObjectiveType::Kill:
			if (Objective.TargetEnemyClass.IsNull())
			{
				return false;
			}
			break;

		case EQuestObjectiveType::Reach:
			if (Objective.TargetLocationTag.IsNone())
			{
				return false;
			}
			break;

		default:
			return false;
		}
	}

	return true;
}

bool UQuestDefinition::HasAnyReward() const
{
	return CurrencyReward > 0.0f
		|| ReputationReward > 0.0f
		|| ItemRewards.Num() > 0
		|| UnlockFlag != NAME_None;
}