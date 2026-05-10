// Source/ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.cpp
#include "ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h"

#include "ShooterGame/Quest/QuestDefinition.h"
#include "ShooterGame/Inventory/StashComponent.h"
#include "ShooterGame/Inventory/ItemDefinition.h"
#include "ShooterGame/Types/ItemTypes.h"

void UQuestTrackerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Pre-populate reputation entries for all vendor roles so UI and systems
    // can always find an entry without null-checking.
    // If a save game is loaded later, LoadFromSaveData() replaces these.
    VendorReputations.Reset();
    for (uint8 i = 0; i < static_cast<uint8>(EVendorRole::Armorer) + 1; ++i)
    {
        FVendorReputationEntry Entry;
        Entry.VendorRole = static_cast<EVendorRole>(i);
        Entry.ReputationLevel = 0.0f;
        VendorReputations.Add(Entry);
    }
}

void UQuestTrackerSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

// ============================================================================
// Quest lifecycle
// ============================================================================

void UQuestTrackerSubsystem::MakeQuestAvailable(UQuestDefinition* QuestDefinition)
{
    if (!IsValid(QuestDefinition))
    {
        return;
    }

    // Guard: do not add if already tracked in any state
    if (FindQuestState(QuestDefinition) != nullptr)
    {
        return;
    }

    FQuestState NewState = BuildInitialState(QuestDefinition, EQuestStatus::Available);
    AvailableQuests.Add(NewState);
    OnQuestStatusChanged.Broadcast(NewState);
}

void UQuestTrackerSubsystem::ActivateQuest(UQuestDefinition* QuestDefinition, UStashComponent* PlayerStash)
{
    if (!IsValid(QuestDefinition))
    {
        return;
    }

    // Find the available entry and promote it to active
    FQuestState* AvailableEntry = FindMutableQuestState(QuestDefinition, EQuestStatus::Available);
    if (!AvailableEntry)
    {
        UE_LOG(LogTemp, Warning, TEXT("UQuestTrackerSubsystem::ActivateQuest — quest not in Available state: %s"),
            *QuestDefinition->Title.ToString());
        return;
    }

    FQuestState ActiveState = *AvailableEntry;
    ActiveState.Status = EQuestStatus::Active;

    // Initialise progress array parallel to the definition's objective array
    ActiveState.ObjectiveProgress.Init(0, QuestDefinition->Objectives.Num());
    ActiveState.bStashCountedOnActivation = false;

    // Pre-fill progress from existing stash items — enforces the design rule
    // that owned quest items never need to be re-farmed.
    if (IsValid(PlayerStash))
    {
        AutoCountStash(ActiveState, PlayerStash);
    }

    const int32 AvailableIdx = AvailableQuests.IndexOfByPredicate([&](const FQuestState& S)
    {
        return S.Definition.Get() == QuestDefinition;
    });
    if (AvailableIdx != INDEX_NONE)
    {
        AvailableQuests.RemoveAt(AvailableIdx);
    }
    
    ActiveQuests.Add(ActiveState);

    OnQuestStatusChanged.Broadcast(ActiveState);

    // Check immediately in case stash already satisfied all objectives
    FQuestState* NewActiveEntry = FindMutableQuestState(QuestDefinition, EQuestStatus::Active);
    if (NewActiveEntry)
    {
        CheckAndTransitionIfComplete(*NewActiveEntry);
    }
}

void UQuestTrackerSubsystem::RecordObjectiveProgress(UQuestDefinition* QuestDefinition, int32 ObjectiveIndex, int32 Delta)
{
    if (!IsValid(QuestDefinition) || Delta <= 0)
    {
        return;
    }

    FQuestState* ActiveEntry = FindMutableQuestState(QuestDefinition, EQuestStatus::Active);
    if (!ActiveEntry)
    {
        return;
    }

    if (!ActiveEntry->ObjectiveProgress.IsValidIndex(ObjectiveIndex))
    {
        return;
    }

    const FQuestObjective& Objective = QuestDefinition->Objectives[ObjectiveIndex];
    ActiveEntry->ObjectiveProgress[ObjectiveIndex] = FMath::Min(
        ActiveEntry->ObjectiveProgress[ObjectiveIndex] + Delta,
        Objective.RequiredCount
    );

    OnQuestProgressUpdated.Broadcast(*ActiveEntry, ObjectiveIndex);
    CheckAndTransitionIfComplete(*ActiveEntry);
}

void UQuestTrackerSubsystem::CompleteQuest(UQuestDefinition* QuestDefinition)
{
    if (!IsValid(QuestDefinition))
    {
        return;
    }

    FQuestState* PendingEntry = FindMutableQuestState(QuestDefinition, EQuestStatus::PendingTurnIn);
    if (!PendingEntry)
    {
        UE_LOG(LogTemp, Warning, TEXT("UQuestTrackerSubsystem::CompleteQuest — quest not in PendingTurnIn state: %s"),
            *QuestDefinition->Title.ToString());
        return;
    }

    FQuestState CompletedState = *PendingEntry;
    CompletedState.Status = EQuestStatus::Completed;

    // Grant reputation to the issuing vendor
    AddReputation(QuestDefinition->IssuingVendor, QuestDefinition->ReputationReward);

    // Register any unlock flag so prerequisite chains and stock gating can query it
    if (!QuestDefinition->UnlockFlag.IsNone())
    {
        ActiveUnlockFlags.AddUnique(QuestDefinition->UnlockFlag);
    }

    const int32 PendingIdx = ActiveQuests.IndexOfByPredicate([&](const FQuestState& S)
    {
        return S.Definition.Get() == QuestDefinition;
    });
    if (PendingIdx != INDEX_NONE)
    {
        ActiveQuests.RemoveAt(PendingIdx);
    }
    
    CompletedQuests.Add(CompletedState);

    OnQuestStatusChanged.Broadcast(CompletedState);

    // NOTE: Currency and item reward payout is intentionally NOT done here.
    // The economy/inventory layer calls CompleteQuest after doing its own
    // server-side validation. Rewards should be granted there, not here,
    // to keep this subsystem decoupled from the economy system.
}

// ============================================================================
// Stash auto-count
// ============================================================================

void UQuestTrackerSubsystem::AutoCountStash(FQuestState& QuestState, UStashComponent* PlayerStash)
{
    if (!IsValid(PlayerStash) || QuestState.bStashCountedOnActivation)
    {
        return;
    }

    UQuestDefinition* ResolvedDef = QuestState.Definition.LoadSynchronous();
    if (!IsValid(ResolvedDef))
    {
        return;
    }

    for (int32 i = 0; i < ResolvedDef->Objectives.Num(); ++i)
    {
        const FQuestObjective& Objective = ResolvedDef->Objectives[i];

        // Only Collect objectives can be pre-filled from stash.
        // Deliver objectives require physical item hand-in, not just ownership.
        if (Objective.ObjectiveType != EQuestObjectiveType::Collect)
        {
            continue;
        }

        if (Objective.TargetItem.IsNull())
        {
            continue;
        }

        UItemDefinition* TargetDef = Objective.TargetItem.LoadSynchronous();
        if (TargetDef == nullptr)
        {
            continue;
        }

        // Count matching items by iterating the stash using GetAllStashItems().
        const TArray<FItemInstance> StashItems = PlayerStash->GetAllStashItems();
        int32 StashCount = 0;
        for (const FItemInstance& StashItem : StashItems)
        {
            if (StashItem.Definition.Get() != nullptr && StashItem.Definition.Get() == TargetDef)
            {
                StashCount += StashItem.StackCount;
            }
        }
        int32 ClampedCount = FMath::Min(StashCount, Objective.RequiredCount);

        if (ClampedCount > QuestState.ObjectiveProgress[i])
        {
            QuestState.ObjectiveProgress[i] = ClampedCount;
            OnQuestProgressUpdated.Broadcast(QuestState, i);
        }
    }

    QuestState.bStashCountedOnActivation = true;
}

// ============================================================================
// Reputation
// ============================================================================

float UQuestTrackerSubsystem::GetReputationFor(EVendorRole VendorRole) const
{
    for (const FVendorReputationEntry& Entry : VendorReputations)
    {
        if (Entry.VendorRole == VendorRole)
        {
            return Entry.ReputationLevel;
        }
    }
    return 0.0f;
}

void UQuestTrackerSubsystem::AddReputation(EVendorRole VendorRole, float Amount)
{
    if (Amount <= 0.0f)
    {
        return;
    }

    for (FVendorReputationEntry& Entry : VendorReputations)
    {
        if (Entry.VendorRole == VendorRole)
        {
            Entry.ReputationLevel = FMath::Clamp(Entry.ReputationLevel + Amount, 0.0f, 100.0f);
            return;
        }
    }

    // Safety: if the entry was somehow missing, create it
    FVendorReputationEntry NewEntry;
    NewEntry.VendorRole = VendorRole;
    NewEntry.ReputationLevel = FMath::Clamp(Amount, 0.0f, 100.0f);
    VendorReputations.Add(NewEntry);
}

// ============================================================================
// Read access / queries
// ============================================================================

const FQuestState* UQuestTrackerSubsystem::FindQuestState(const UQuestDefinition* QuestDefinition) const
{
    if (!IsValid(QuestDefinition))
    {
        return nullptr;
    }

    auto Search = [&](const TArray<FQuestState>& Array) -> const FQuestState*
    {
        for (const FQuestState& State : Array)
        {
            if (State.Definition.Get() == QuestDefinition)
            {
                return &State;
            }
        }
        return nullptr;
    };

    if (const FQuestState* Found = Search(ActiveQuests))   { return Found; }
    if (const FQuestState* Found = Search(AvailableQuests)){ return Found; }
    if (const FQuestState* Found = Search(CompletedQuests)){ return Found; }

    return nullptr;
}

bool UQuestTrackerSubsystem::IsQuestCompleted(UQuestDefinition* QuestDefinition) const
{
    if (!IsValid(QuestDefinition))
    {
        return false;
    }

    for (const FQuestState& State : CompletedQuests)
    {
        if (State.Definition.Get() == QuestDefinition)
        {
            return true;
        }
    }
    return false;
}

bool UQuestTrackerSubsystem::IsUnlockFlagActive(FName UnlockFlag) const
{
    if (UnlockFlag.IsNone())
    {
        return false;
    }
    return ActiveUnlockFlags.Contains(UnlockFlag);
}

// ============================================================================
// Save integration hooks
// ============================================================================

void UQuestTrackerSubsystem::LoadFromSaveData(
    const TArray<FQuestState>& SavedActive,
    const TArray<FQuestState>& SavedAvailable,
    const TArray<FQuestState>& SavedCompleted,
    const TArray<FVendorReputationEntry>& SavedReputations)
{
    ActiveQuests    = SavedActive;
    AvailableQuests = SavedAvailable;
    CompletedQuests = SavedCompleted;
    VendorReputations = SavedReputations;

    // Rebuild unlock flag set from completed quests
    ActiveUnlockFlags.Reset();
    for (const FQuestState& State : CompletedQuests)
    {
        UQuestDefinition* Def = State.Definition.LoadSynchronous();
        if (IsValid(Def) && !Def->UnlockFlag.IsNone())
        {
            ActiveUnlockFlags.AddUnique(Def->UnlockFlag);
        }
    }
}

void UQuestTrackerSubsystem::GetSaveData(
    TArray<FQuestState>& OutActive,
    TArray<FQuestState>& OutAvailable,
    TArray<FQuestState>& OutCompleted,
    TArray<FVendorReputationEntry>& OutReputations) const
{
    OutActive      = ActiveQuests;
    OutAvailable   = AvailableQuests;
    OutCompleted   = CompletedQuests;
    OutReputations = VendorReputations;
}

// ============================================================================
// Private helpers
// ============================================================================

FQuestState* UQuestTrackerSubsystem::FindMutableQuestState(const UQuestDefinition* QuestDefinition, EQuestStatus InStatus)
{
    TArray<FQuestState>* TargetArray = nullptr;

    switch (InStatus)
    {
    case EQuestStatus::Active:        TargetArray = &ActiveQuests;    break;
    case EQuestStatus::Available:     TargetArray = &AvailableQuests; break;
    case EQuestStatus::PendingTurnIn: TargetArray = &ActiveQuests;    break;
    case EQuestStatus::Completed:     TargetArray = &CompletedQuests; break;
    default:                          return nullptr;
    }

    for (FQuestState& State : *TargetArray)
    {
        if (State.Definition.Get() == QuestDefinition)
        {
            return &State;
        }
    }

    return nullptr;
}

FQuestState UQuestTrackerSubsystem::BuildInitialState(UQuestDefinition* QuestDefinition, EQuestStatus InitialStatus)
{
    FQuestState State;
    State.Definition = QuestDefinition;
    State.Status = InitialStatus;
    State.bStashCountedOnActivation = false;

    // Objective progress is zeroed here; AutoCountStash fills it on activation
    if (IsValid(QuestDefinition))
    {
        State.ObjectiveProgress.Init(0, QuestDefinition->Objectives.Num());
    }

    return State;
}

void UQuestTrackerSubsystem::CheckAndTransitionIfComplete(FQuestState& QuestState)
{
    UQuestDefinition* ResolvedDef = QuestState.Definition.LoadSynchronous();
    if (!IsValid(ResolvedDef))
    {
        return;
    }

    if (!QuestState.AreAllObjectivesComplete(ResolvedDef))
    {
        return;
    }

    QuestState.Status = EQuestStatus::PendingTurnIn;
    OnQuestStatusChanged.Broadcast(QuestState);
}