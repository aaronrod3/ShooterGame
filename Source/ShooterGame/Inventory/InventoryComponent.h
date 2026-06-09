// Source/ShooterGame/Inventory/InventoryComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "ShooterGame/Types/AmmoType.h"
#include "InventoryComponent.generated.h"

// Broadcast when the Items array changes on the client (OnRep fires this).
// UI widgets bind to this delegate — they never poll the array directly.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

// Broadcast when a single item's runtime state changes (durability, ammo count).
// Carries the InstanceID of the affected item so the UI can update only that slot.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemStateChanged, FGuid, InstanceID);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// -----------------------------------------------------------------------
	// Delegates — UI binds here, never polls the array
	// -----------------------------------------------------------------------

	// Fired on clients whenever the Items array is replicated.
	// Bind your inventory widget's full refresh here.
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	// Fired when a single item's state changes (durability tick, ammo update).
	// Bind individual item slot widgets here for lightweight refreshes.
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemStateChanged OnItemStateChanged;

	// -----------------------------------------------------------------------
	// Server-Authoritative Item Interface
	// All mutations below must be called on the server. They return false
	// immediately if called on a client — the client is a view layer only.
	// -----------------------------------------------------------------------

	/**
	 * Attempts to add an item instance to this inventory.
	 * Server only. Validates slot tag compatibility and capacity before adding.
	 * Assigns a new InstanceID if the instance does not already have one.
	 * Returns true if the item was accepted and added.
	 *
	 * SlotTag: the RequiredSlotTag of the container being added to.
	 *          Pass FGameplayTag() (empty) to skip tag validation (e.g. stash has no tag restriction).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (AutoCreateRefTerm = "SlotTag"))
	bool Server_AddItem(FItemInstance& ItemInstance, const FGameplayTag& SlotTag);

	/**
	 * Removes the item with the given InstanceID from this inventory.
	 * Server only. Returns the removed instance via OutItem (for transfer to another container).
	 * Returns false if the InstanceID was not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Server_RemoveItem(const FGuid& InstanceID, FItemInstance& OutItem);

	/**
	 * Transfers an item from this inventory to a target inventory.
	 * Server only. Validates tag compatibility on the target before committing.
	 * Atomic — the item is never lost if the target rejects it (it stays here).
	 * Returns true if the transfer completed successfully.
	 *
	 * TargetSlotTag: the RequiredSlotTag of the target container slot.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (AutoCreateRefTerm = "TargetSlotTag"))
	bool Server_TransferItemTo(const FGuid& InstanceID, UInventoryComponent* TargetInventory, const FGameplayTag& TargetSlotTag);

	/**
	 * Updates the runtime state of an existing item (durability, ammo count).
	 * Server only. Finds the item by InstanceID and applies the new state.
	 * Broadcasts OnItemStateChanged after the update so the UI refreshes only
	 * the affected slot rather than doing a full inventory redraw.
	 * Returns false if the InstanceID was not found.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool Server_UpdateItemState(const FGuid& InstanceID, const FItemInstance& UpdatedInstance);

	// -----------------------------------------------------------------------
	// Read-Only Accessors (safe to call on client and server)
	// -----------------------------------------------------------------------

	/**
	 * Returns a const pointer to the item with the given InstanceID.
	 * Returns nullptr if not found. Pointer is valid until Items is modified —
	 * use immediately, do not cache across frames.
	 */
	const FItemInstance* FindItemByID(const FGuid& InstanceID) const;

	/**
	 * Returns true if this inventory contains an item with the given InstanceID.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ContainsItem(const FGuid& InstanceID) const;

	/**
	 * Returns true if this inventory contains at least one item whose Definition
	 * matches the given asset. Used by quest system to check stash ownership.
	 * Quest items already in stash count — the player never re-farms owned items.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool ContainsDefinition(const class UItemDefinition* Definition) const;

	/**
	 * Returns true if this inventory has room for at least one more item.
	 * Pass MaxItems = 0 in the constructor/defaults for unlimited capacity.
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasCapacity() const;

	/**
	 * Returns true if an item with the given AcceptedSlotTags is valid
	 * for a slot that requires RequiredSlotTag.
	 * An empty RequiredSlotTag always returns true (untagged slot accepts anything).
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (AutoCreateRefTerm = "RequiredSlotTag"))
	static bool IsItemValidForSlot(const FItemInstance& Item, const FGameplayTag& RequiredSlotTag);

	// Read-only access to the full item array (e.g. for full UI rebuild on OnRep)
	FORCEINLINE const TArray<FItemInstance>& GetItems() const { return Items; }
	FORCEINLINE int32 GetItemCount()                   const { return Items.Num(); }
	FORCEINLINE int32 GetMaxItems()                    const { return MaxItems; }

	// -----------------------------------------------------------------------
	// Magazine Shim — preserves UCombatComponent / AAmmoPickup compatibility
	// These functions delegate to the Items array internally.
	// They will be deprecated once ammo is fully migrated to FItemInstance.
	// -----------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddMagazine(const FMagazine& Mag);

	FMagazine* GetBestMagazine(EAmmoType AmmoType);

	bool RemoveMagazine(const FMagazine& Mag);

	bool ReturnMagazine(const FMagazine& Mag);
	
	// Removes all magazines from MagazineSlots.
	// Called by OnLoadoutChanged_Internal before granting fresh starting mags.
	void ClearAllMagazines();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 CountMagazinesOfType(EAmmoType AmmoType) const;

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasMagazineFor(EAmmoType AmmoType) const;

	FORCEINLINE int32 GetMaxMagazineSlots()  const { return MaxMagazineSlots; }
	FORCEINLINE int32 GetUsedMagazineSlots() const { return MagazineSlots.Num(); }
	FORCEINLINE bool  IsMagazineFull()       const { return MagazineSlots.Num() >= MaxMagazineSlots; }
	FORCEINLINE const TArray<FMagazine>& GetMagazineSlots() const { return MagazineSlots; }

protected:
	virtual void BeginPlay() override;
	
	
	// Allows derived inventory types (such as stash, rig, or backpack specializations)
	// to configure default capacity rules in C++ constructors without exposing
	// writable capacity state to external callers.
	void SetMaxItems(int32 NewMaxItems) { MaxItems = FMath::Max(0, NewMaxItems); }

private:

	// -----------------------------------------------------------------------
	// General Item Container
	// -----------------------------------------------------------------------

	/**
	 * The canonical item array for this inventory container.
	 * Server is the authority — clients receive this via replication.
	 * UI always reads from this array via GetItems() or FindItemByID().
	 * Never modify this array directly outside of Server_ functions.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_Items)
	TArray<FItemInstance> Items;

	UFUNCTION()
	void OnRep_Items();

	/**
	 * Maximum number of item instances this container can hold.
	 * 0 = unlimited (useful for player stash).
	 * Set in Blueprint defaults per container type:
	 *   Stash = 0 (unlimited), Rig = 12, Backpack = 30, Belt = 6
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory",
		meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 MaxItems = 30;

	// Internal mutable find — used by Server_ functions to get a writable pointer.
	FItemInstance* FindItemByID_Mutable(const FGuid& InstanceID);

	// -----------------------------------------------------------------------
	// Magazine Shim Storage (legacy — coexists until ammo migration)
	// -----------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory",
		meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 MaxMagazineSlots = 6;

	UPROPERTY(ReplicatedUsing = OnRep_MagazineSlots)
	TArray<FMagazine> MagazineSlots;

	UFUNCTION()
	void OnRep_MagazineSlots();
};