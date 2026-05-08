// Source/ShooterGame/Types/ContainerTypes.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ContainerTypes.generated.h"

// ============================================================================
// EEquipmentContainerType
//
// Identifies the major persistent inventory/equipment containers a player uses.
// This is used by save data, UI routing, and server-side validation logic.
// ============================================================================

UENUM(BlueprintType)
enum class EEquipmentContainerType : uint8
{
	None        UMETA(DisplayName = "None"),
	Stash       UMETA(DisplayName = "Stash"),
	Rig         UMETA(DisplayName = "Rig"),
	Belt        UMETA(DisplayName = "Belt"),
	Backpack    UMETA(DisplayName = "Backpack"),
	Mission     UMETA(DisplayName = "Mission Runtime")
};


// ============================================================================
// FEquipmentContainerConfig
//
// Data-driven config for a container's slot count, weight budget, and slot tag.
// Weight never blocks placement by design, but we still track MaxWeightKg here
// because runtime burden calculations depend on container contents.
// MaxSlots == 0 means unlimited.
// ============================================================================

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FEquipmentContainerConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	FText DisplayName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	EEquipmentContainerType ContainerType = EEquipmentContainerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container", meta = (ClampMin = "0"))
	int32 MaxSlots = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container", meta = (ClampMin = "0.0"))
	float MaxWeightKg = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	FGameplayTag RequiredSlotTag;
};


// ============================================================================
// FNamedEquippedItem
//
// Represents one dedicated body/equipment slot that can hold exactly one item.
// Example: Primary weapon, Helmet, Chest armor, Knife.
// ============================================================================

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FNamedEquippedItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FItemInstance ItemInstance;

	bool IsOccupied() const
	{
		return ItemInstance.IsValid();
	}

	void Clear()
	{
		ItemInstance = FItemInstance();
	}
};


// ============================================================================
// FEquippedContainerState
//
// Snapshot of one multi-item carried container, such as Rig/Belt/Backpack.
// This is save-friendly data, not a live component.
// ============================================================================

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FEquippedContainerState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	FEquipmentContainerConfig Config;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Container")
	TArray<FItemInstance> Items;

	float GetTotalWeight() const;
	int32 GetUsedSlots() const { return Items.Num(); }
};


// ============================================================================
// FEquippedStateSnapshot
//
// Serialized snapshot of the player's carried/equipped state.
// This is the exact state that persists after extraction until the player
// manually reorganizes it in the lobby.
// ============================================================================

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FEquippedStateSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem PrimaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem SecondaryWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem SidearmWeapon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem HelmetItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem ChestItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem ToolItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FNamedEquippedItem KnifeItem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FEquippedContainerState RigState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FEquippedContainerState BeltState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
	FEquippedContainerState BackpackState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (ClampMin = "0.0"))
	float CurrentCarriedWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (ClampMin = "0.0"))
	float CurrentBurdenScore = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment", meta = (ClampMin = "0.0"))
	float DeadPlayerBurdenMultiplier = 1.0f;

	void ClearAll();
	float CalculateTotalWeight() const;
};


// ============================================================================
// FLoadoutPresetSlot
//
// A preset entry references an item by InstanceID only. When applying a preset,
// later systems will resolve the InstanceID from the stash and flag missing items
// rather than silently failing.
// ============================================================================

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FLoadoutPresetSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FGuid ItemInstanceID;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Preset")
	bool bItemMissingFromStash = false;

	bool HasAssignedItem() const
	{
		return ItemInstanceID.IsValid();
	}

	void Clear()
	{
		ItemInstanceID.Invalidate();
		bItemMissingFromStash = false;
	}
};


// ============================================================================
// FLoadoutPreset
//
// One named preset slot. It references items by InstanceID and stores desired
// contents for both dedicated body slots and list-based carried containers.
// ============================================================================

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FLoadoutPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	FString PresetName = TEXT("New Preset");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TArray<FLoadoutPresetSlot> EquippedSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TArray<FGuid> RigItemIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TArray<FGuid> BeltItemIDs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Preset")
	TArray<FGuid> BackpackItemIDs;

	void InitializeDefaultSlots();
	void ClearAll();

	bool HasAnyAssignedItems() const
	{
		return EquippedSlots.Num() > 0 || RigItemIDs.Num() > 0 || BeltItemIDs.Num() > 0 || BackpackItemIDs.Num() > 0;
	}
};