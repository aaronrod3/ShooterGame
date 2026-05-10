// Source/ShooterGame/HUD/Inventory/VendorWidget.cpp
#include "ShooterGame/HUD/Inventory/VendorWidget.h"

#include "ShooterGame/Interaction/VendorNPCActor.h"
#include "ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h"
#include "ShooterGame/Quest/QuestDefinition.h"

void UVendorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    QuestTrackerSubsystem = GetGameInstance()->GetSubsystem<UQuestTrackerSubsystem>();

    // FUTURE: When UQuestTrackerSubsystem exposes a rep-changed delegate,
    // bind here to call RefreshStock() automatically on rep gain.
}

void UVendorWidget::NativeDestruct()
{
    CurrentVendor = nullptr;
    Super::NativeDestruct();
}

void UVendorWidget::InitVendor(AVendorNPCActor* Vendor)
{
    if (!IsValid(Vendor))
    {
        return;
    }

    CurrentVendor = Vendor;
    RefreshStock();
    RefreshQuestList();
}

void UVendorWidget::RefreshStock()
{
    if (!IsValid(CurrentVendor))
    {
        return;
    }

    const float CurrentRep = GetCurrentReputation();
    const TArray<FVendorInventoryEntry>& AllStock = CurrentVendor->GetVendorStock();

    // Filter stock by reputation threshold.
    // Server re-validates before any purchase — this is display-only filtering.
    TArray<FVendorInventoryEntry> FilteredStock;
    FilteredStock.Reserve(AllStock.Num());

    for (const FVendorInventoryEntry& Entry : AllStock)
    {
        if (CurrentRep >= Entry.MinReputationRequired)
        {
            FilteredStock.Add(Entry);
        }
    }

    OnStockRefreshed_BP(FilteredStock);
}

void UVendorWidget::RefreshQuestList()
{
    if (!IsValid(CurrentVendor))
    {
        return;
    }

    TArray<UQuestDefinition*> AvailableQuests = CurrentVendor->GetAvailableQuests();
    OnQuestListRefreshed_BP(AvailableQuests);
}

void UVendorWidget::RequestPurchase(const FVendorInventoryEntry& Entry)
{
    if (!IsValid(CurrentVendor))
    {
        return;
    }

    OnPurchaseRequested_BP(Entry);

    // FUTURE: Call CurrentVendor->ServerPurchaseItem(Entry) here when the
    // server RPC is implemented in AVendorNPCActor (planned for a later step).
    // The delegate fires now so UI feedback and animations can be built today.
}

void UVendorWidget::RequestSell(const FItemInstance& Item)
{
    if (!IsValid(CurrentVendor))
    {
        return;
    }

    OnSellRequested_BP(Item);

    // FUTURE: Call CurrentVendor->ServerSellItem(Item) here when implemented.
}

void UVendorWidget::RequestAcceptQuest(UQuestDefinition* QuestDefinition)
{
    if (!IsValid(QuestDefinition) || !QuestTrackerSubsystem)
    {
        return;
    }

    // MakeQuestAvailable ensures it's in the Available list, then ActivateQuest
    // promotes it to Active and runs AutoCountStash.
    // PlayerStash is not available from the widget layer — passing nullptr here
    // means AutoCountStash won't run until the character's stash ref is plumbed in.
    //
    // FUTURE: Resolve the local player's UStashComponent here and pass it in.
    QuestTrackerSubsystem->MakeQuestAvailable(QuestDefinition);
    QuestTrackerSubsystem->ActivateQuest(QuestDefinition, nullptr);

    RefreshQuestList();
}

void UVendorWidget::RequestTurnInQuest(UQuestDefinition* QuestDefinition)
{
    if (!IsValid(QuestDefinition) || !QuestTrackerSubsystem)
    {
        return;
    }

    if (!QuestTrackerSubsystem->IsQuestCompleted(QuestDefinition))
    {
        QuestTrackerSubsystem->CompleteQuest(QuestDefinition);
    }

    RefreshQuestList();
}

EVendorRole UVendorWidget::GetCurrentVendorRole() const
{
    if (!IsValid(CurrentVendor))
    {
        return EVendorRole::Quartermaster;
    }
    return CurrentVendor->GetVendorRole();
}

float UVendorWidget::GetCurrentReputation() const
{
    if (!QuestTrackerSubsystem || !IsValid(CurrentVendor))
    {
        return 0.0f;
    }
    return QuestTrackerSubsystem->GetReputationFor(CurrentVendor->GetVendorRole());
}