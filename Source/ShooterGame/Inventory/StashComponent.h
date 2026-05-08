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

protected:
	virtual void BeginPlay() override;
};