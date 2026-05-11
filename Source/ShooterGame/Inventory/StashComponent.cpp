// Source/ShooterGame/Inventory/StashComponent.cpp
#include "ShooterGame/Inventory/StashComponent.h"
#include "ShooterGame/Inventory/ItemDefinition.h"

UStashComponent::UStashComponent()
{
	// Stash is unlimited capacity — MaxItems 0 = no cap (see InventoryComponent)
	SetMaxItems(0);
}

void UStashComponent::BeginPlay()
{
	Super::BeginPlay();
}

TArray<FItemInstance> UStashComponent::GetItemsByCategory(EItemCategory Category) const
{
	TArray<FItemInstance> Result;
	for (const FItemInstance& Item : GetItems())
	{
		if (IsValid(Item.Definition) && Item.Definition->ItemCategory == Category)
		{
			Result.Add(Item);
		}
	}
	return Result;
}

TArray<FItemInstance> UStashComponent::GetAllStashItems() const
{
	return GetItems();
}

// ---------------------------------------------------------------------------
// Currency — CountCurrencyInStash
// ---------------------------------------------------------------------------

int32 UStashComponent::CountCurrencyInStash() const
{
	int32 Total = 0;
	for (const FItemInstance& Item : GetItems())
	{
		if (IsValid(Item.Definition) && Item.Definition->bIsCurrency)
		{
			Total += Item.StackCount * Item.Definition->CurrencyValue;
		}
	}
	return Total;
}

// ---------------------------------------------------------------------------
// Currency — SpendCurrencyFromStash
// ---------------------------------------------------------------------------

bool UStashComponent::SpendCurrencyFromStash(int32 Amount)
{
	// Server only — never trust client-side economy mutations
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpendCurrencyFromStash: called on client — ignoring."));
		return false;
	}

	if (Amount <= 0)
	{
		return true; // Nothing to spend — trivially successful
	}

	// --- Step 1: Atomic balance check before touching anything ---
	if (CountCurrencyInStash() < Amount)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpendCurrencyFromStash: insufficient funds. Have %d, need %d."),
			CountCurrencyInStash(), Amount);
		return false;
	}

	// --- Step 2: Collect all currency stacks, sort descending by CurrencyValue ---
	// Highest denomination first minimises the number of stacks touched per transaction
	TArray<FItemInstance> CurrencyStacks;
	for (const FItemInstance& Item : GetItems())
	{
		if (IsValid(Item.Definition) && Item.Definition->bIsCurrency)
		{
			CurrencyStacks.Add(Item);
		}
	}

	CurrencyStacks.Sort([](const FItemInstance& A, const FItemInstance& B)
	{
		const int32 ValA = IsValid(A.Definition) ? A.Definition->CurrencyValue : 0;
		const int32 ValB = IsValid(B.Definition) ? B.Definition->CurrencyValue : 0;
		return ValA > ValB; // Descending — highest value first
	});

	// --- Step 3: Drain stacks until Amount is satisfied ---
	int32 Remaining = Amount;

	for (const FItemInstance& Stack : CurrencyStacks)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (!IsValid(Stack.Definition))
		{
			continue;
		}

		const int32 StackWorth   = Stack.StackCount * Stack.Definition->CurrencyValue;
		const int32 UnitsNeeded  = FMath::DivideAndRoundUp(Remaining, Stack.Definition->CurrencyValue);
		const int32 UnitsToSpend = FMath::Min(UnitsNeeded, Stack.StackCount);

		if (UnitsToSpend >= Stack.StackCount)
		{
			// Consume the entire stack
			FItemInstance Removed;
			Server_RemoveItem(Stack.InstanceID, Removed);
			Remaining -= StackWorth;
		}
		else
		{
			// Partially consume this stack — mutate StackCount in place
			FItemInstance Updated = Stack;
			Updated.StackCount   -= UnitsToSpend;
			Server_UpdateItemState(Stack.InstanceID, Updated);
			Remaining -= UnitsToSpend * Stack.Definition->CurrencyValue;
		}
	}

	// Remaining should be <= 0 here due to the upfront balance check
	// A negative Remaining means we overpaid due to denomination rounding —
	// that is intentional and matches EFT behaviour (no change given)
	return true;
}

// ---------------------------------------------------------------------------
// Currency — AwardCurrencyToStash
// ---------------------------------------------------------------------------

bool UStashComponent::AwardCurrencyToStash(int32 Amount, UItemDefinition* CurrencyDefinition)
{
	// Server only
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("AwardCurrencyToStash: called on client — ignoring."));
		return false;
	}

	if (Amount <= 0 || !IsValid(CurrencyDefinition) || !CurrencyDefinition->bIsCurrency)
	{
		UE_LOG(LogTemp, Warning, TEXT("AwardCurrencyToStash: invalid arguments."));
		return false;
	}

	const int32 StackMax   = CurrencyDefinition->StackMax;
	int32       Remaining  = Amount;

	// --- Step 1: Top up existing stacks of the same Definition first ---
	// Mirrors EFT stack-merge — avoids stash clutter from repeated sell transactions
	for (const FItemInstance& Item : GetItems())
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Item.Definition != CurrencyDefinition)
		{
			continue;
		}

		const int32 Headroom = StackMax - Item.StackCount;
		if (Headroom <= 0)
		{
			continue;
		}

		const int32 ToAdd  = FMath::Min(Remaining, Headroom);
		FItemInstance Updated = Item;
		Updated.StackCount   += ToAdd;
		Server_UpdateItemState(Item.InstanceID, Updated);
		Remaining -= ToAdd;
	}

	// --- Step 2: Create new stacks for any remainder ---
	while (Remaining > 0)
	{
		const int32 ThisStack = FMath::Min(Remaining, StackMax);

		FItemInstance NewStack;
		NewStack.Definition  = CurrencyDefinition;
		NewStack.StackCount  = ThisStack;
		NewStack.bIsQuestItem = false;
		NewStack.AssignNewInstanceID();

		// Empty slot tag — stash has no tag restriction (see InventoryComponent)
		const FGameplayTag EmptyTag;
		if (!Server_AddItem(NewStack, EmptyTag))
		{
			UE_LOG(LogTemp, Error, TEXT("AwardCurrencyToStash: Server_AddItem failed — stash may be full."));
			return false;
		}

		Remaining -= ThisStack;
	}

	return true;
}