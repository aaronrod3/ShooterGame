// Source/ShooterGame/Types/ItemContextMenuTypes.h
#pragma once

#include "CoreMinimal.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "ShooterGame/Types/ItemTypes.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "ItemContextMenuTypes.generated.h"

UENUM(BlueprintType)
enum class EItemContextAction : uint8
{
	None            UMETA(DisplayName = "None"),
	Equip           UMETA(DisplayName = "Equip"),
	MoveToStash     UMETA(DisplayName = "Move To Stash"),
	MoveToBackpack  UMETA(DisplayName = "Move To Backpack"),
	SplitStack      UMETA(DisplayName = "Split Stack"),
	MergeStack      UMETA(DisplayName = "Merge Stack"),
	Inspect         UMETA(DisplayName = "Inspect"),
	Drop            UMETA(DisplayName = "Drop")
};

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FItemContextMenuData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	FItemInstance ItemInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	EEquipmentContainerType SourceContainerType = EEquipmentContainerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	EEquipmentSlot SourceEquipmentSlot = EEquipmentSlot::MAX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowEquip = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowMoveToStash = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowMoveToBackpack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowSplitStack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowMergeStack = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowInspect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Context Menu")
	bool bAllowDrop = false;
};