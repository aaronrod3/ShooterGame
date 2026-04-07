// Source/ShooterGame/Components/InventoryComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "ShooterGame/Types/AmmoType.h"
#include "InventoryComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// -----------------------------------------------------------------------
	// Magazine Slot Interface
	// -----------------------------------------------------------------------

	/**
	 * Adds a magazine to the first available slot.
	 * Returns true if added, false if slots are full.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddMagazine(const FMagazine& Mag);

	/**
	 * Finds the magazine with the most rounds for the given ammo type.
	 * Returns a pointer to the slot entry, or nullptr if none found.
	 * Pointer is valid until MagazineSlots is modified — use immediately.
	 */
	FMagazine* GetBestMagazine(EAmmoType AmmoType);

	/**
	 * Removes a specific magazine from inventory by value match.
	 * Returns true if found and removed.
	 */
	bool RemoveMagazine(const FMagazine& Mag);

	/**
	 * Returns a partially-used ejected magazine back to inventory.
	 * Empty magazines are discarded — they take a slot for nothing.
	 * Returns true if the magazine was kept.
	 */
	bool ReturnMagazine(const FMagazine& Mag);

	/**
	 * Returns how many magazines of a given ammo type are available
	 * (non-empty only).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 CountMagazinesOfType(EAmmoType AmmoType) const;

	/**
	 * Returns true if any non-empty magazine of the given type exists.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasMagazineFor(EAmmoType AmmoType) const;

	// -----------------------------------------------------------------------
	// Accessors
	// -----------------------------------------------------------------------

	FORCEINLINE int32 GetMaxMagazineSlots()  const { return MaxMagazineSlots; }
	FORCEINLINE int32 GetUsedMagazineSlots() const { return MagazineSlots.Num(); }
	FORCEINLINE bool  IsMagazineFull()       const { return MagazineSlots.Num() >= MaxMagazineSlots; }

	// Read-only access to the full slot array (e.g. for HUD display)
	FORCEINLINE const TArray<FMagazine>& GetMagazineSlots() const { return MagazineSlots; }

protected:
	virtual void BeginPlay() override;

private:

	/**
	 * Maximum number of magazine slots available to this character.
	 * Set this per character class or loadout in the blueprint defaults.
	 * e.g. light class = 4, heavy class = 8
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxMagazineSlots = 6;

	/**
	 * The actual magazine objects held in equipment slots.
	 * Replicated so all clients see the correct inventory state.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_MagazineSlots)
	TArray<FMagazine> MagazineSlots;

	UFUNCTION()
	void OnRep_MagazineSlots();
};