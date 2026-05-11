// Source/ShooterGame/Inventory/StashComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Inventory/InventoryComponent.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "StashComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UStashComponent : public UInventoryComponent
{
	GENERATED_BODY()

public:
	UStashComponent();

	// Returns all items in the stash that belong to the requested category.
	// Category filtering is read-only and intended for stash UI tabs.
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash")
	TArray<FItemInstance> GetItemsByCategory(EItemCategory Category) const;

	// Returns all items in the stash.
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash")
	TArray<FItemInstance> GetAllStashItems() const;

	// -----------------------------------------------------------------------
	// Currency Helpers (Phase 4)
	// -----------------------------------------------------------------------

	/**
	 * Returns the total currency balance in the stash, expressed in base units.
	 * Sums StackCount * Definition->CurrencyValue for every item where
	 * Definition->bIsCurrency == true.
	 *
	 * Safe to call on both server and client (read-only).
	 * Only the server result is authoritative — use for UI display on client,
	 * use for validation on server before calling SpendCurrencyFromStash().
	 *
	 * Returns 0 if no currency items are present.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash|Currency")
	int32 CountCurrencyInStash() const;

	/**
	 * Spends the requested Amount of currency from the stash. Server only.
	 *
	 * Deducts from stacks in descending CurrencyValue order (highest-value
	 * currency consumed first — minimises stack fragmentation).
	 * Fully depleted stacks are removed via Server_RemoveItem.
	 * Partially consumed stacks have their StackCount decremented in-place.
	 *
	 * ATOMIC: performs a full balance check before touching any stack.
	 * Returns false immediately (no mutation) if CountCurrencyInStash() < Amount.
	 * Returns true and commits all mutations on success.
	 *
	 * Server only — returns false immediately on client.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash|Currency")
	bool SpendCurrencyFromStash(int32 Amount);

	/**
	 * Awards the given Amount of currency to the stash by adding or growing
	 * a stack of the specified CurrencyDefinition. Server only.
	 *
	 * Attempts to top up an existing stack of the same Definition before
	 * creating a new FItemInstance — mirrors EFT's stack-merge behaviour.
	 * If the existing stack is full (StackCount == StackMax), a new stack
	 * is created. If no Definition is provided the call is a no-op.
	 *
	 * Server only — returns false immediately on client.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash|Currency")
	bool AwardCurrencyToStash(int32 Amount, UItemDefinition* CurrencyDefinition);

protected:
	virtual void BeginPlay() override;
};