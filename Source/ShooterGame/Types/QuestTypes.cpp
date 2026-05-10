// Source/ShooterGame/Types/QuestTypes.cpp
#include "QuestTypes.h"
#include "ShooterGame/Quest/QuestDefinition.h"

bool FQuestState::AreAllObjectivesComplete(const UQuestDefinition* ResolvedDef) const
{
	if (!ResolvedDef)
	{
		return false;
	}

	// ObjectiveProgress must be in sync with the definition's objective count
	if (ObjectiveProgress.Num() != ResolvedDef->Objectives.Num())
	{
		return false;
	}

	for (int32 i = 0; i < ResolvedDef->Objectives.Num(); ++i)
	{
		if (ObjectiveProgress[i] < ResolvedDef->Objectives[i].RequiredCount)
		{
			return false;
		}
	}

	return true;
}