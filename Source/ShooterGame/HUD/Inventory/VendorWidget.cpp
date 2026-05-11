// Source/ShooterGame/HUD/Inventory/VendorWidget.cpp
#include "ShooterGame/HUD/Inventory/VendorWidget.h"

#include "ShooterGame/Interaction/VendorNPCActor.h"
#include "ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h"
#include "ShooterGame/Quest/QuestDefinition.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "GameFramework/PlayerController.h"

void UVendorWidget::NativeConstruct()
{
    Super::NativeConstruct();

    QuestTrackerSubsystem = GetGameInstance()->GetSubsystem<UQuestTrackerSubsystem>();

    // Cache the owning character once — used for every RPC dispatch.
    // GetOwningPlayerPawn() returns the possessed pawn for the local player
    // that owns this widget, which is always AShooterGameCharacter in this project.
    OwningCharacter = Cast<AShooterGameCharacter>(GetOwningPlayerPawn());

    if (!OwningCharacter)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UVendorWidget::NativeConstruct — could not resolve owning AShooterGameCharacter. "
                 "Buy/sell RPCs will not fire."));
    }

    // FUTURE: When UQuestTrackerSubsystem exposes a rep-changed delegate,
    // bind here to call RefreshStock() automatically on rep gain.
}

void UVendorWidget::NativeDestruct()
{
    CurrentVendor    = nullptr;
    OwningCharacter  = nullptr;
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
    if (!IsValid(CurrentVendor) || !IsValid(OwningCharacter))
    {
        return;
    }

    // Resolve the stock index by matching the entry against the vendor's
    // full stock array. The server validates this index independently —
    // we never trust the client to pass a pre-validated entry struct.
    const TArray<FVendorInventoryEntry>& Stock = CurrentVendor->GetVendorStock();
    int32 EntryIndex = INDEX_NONE;
    for (int32 i = 0; i < Stock.Num(); ++i)
    {
        // Match by ItemDefinition pointer — each stock entry has a unique asset ref
        if (Stock[i].ItemDefinition == Entry.ItemDefinition)
        {
            EntryIndex = i;
            break;
        }
    }

    if (EntryIndex == INDEX_NONE)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UVendorWidget::RequestPurchase — entry not found in vendor stock. Aborting."));
        return;
    }

    // Fire pending BP event so the designer can play a loading animation
    // while the round trip to the server completes.
    OnPurchaseRequested_BP(Entry);

    // Dispatch the server RPC — all real validation happens server-side.
    OwningCharacter->ServerPurchaseItem(CurrentVendor, EntryIndex);
}

void UVendorWidget::RequestSell(const FItemInstance& Item)
{
    if (!IsValid(CurrentVendor) || !IsValid(OwningCharacter))
    {
        return;
    }

    if (!Item.InstanceID.IsValid())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("UVendorWidget::RequestSell — item has invalid InstanceID. Aborting."));
        return;
    }

    // Fire pending BP event for loading/pending animation.
    OnSellRequested_BP(Item);

    // Dispatch the server RPC — item is identified by GUID, never by array index.
    OwningCharacter->ServerSellItem(CurrentVendor, Item.InstanceID);
}

void UVendorWidget::HandleTransactionResult(const FVendorTransactionResult& Result)
{
    // On success: refresh the stock panel so depleted items disappear
    // and the player's updated stash reflects the purchase immediately.
    if (Result.bSuccess)
    {
        RefreshStock();
    }

    // Always forward to BP — designer handles success animation and error toast.
    OnTransactionResult_BP(Result);
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
    // Step 10 resolves the stash ref and passes it in properly.
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