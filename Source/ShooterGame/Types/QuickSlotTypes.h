// Source/ShooterGame/Types/QuickSlotTypes.h
#pragma once

#include "CoreMinimal.h"
#include "QuickSlotTypes.generated.h"

UENUM(BlueprintType)
enum class EQuickSlotCategory : uint8
{
	None        UMETA(DisplayName = "None"),
	Weapon      UMETA(DisplayName = "Weapon"),
	Tool        UMETA(DisplayName = "Tool"),
	Consumable  UMETA(DisplayName = "Consumable")
};

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FQuickSlotAssignment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quick Slot")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quick Slot")
	EQuickSlotCategory Category = EQuickSlotCategory::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quick Slot")
	FGuid ItemInstanceID;

	bool IsAssigned() const
	{
		return SlotIndex != INDEX_NONE && ItemInstanceID.IsValid();
	}

	void Clear()
	{
		SlotIndex = INDEX_NONE;
		Category = EQuickSlotCategory::None;
		ItemInstanceID.Invalidate();
	}
};