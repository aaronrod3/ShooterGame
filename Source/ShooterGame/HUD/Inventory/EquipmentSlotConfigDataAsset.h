// Source/ShooterGame/HUD/Inventory/EquipmentSlotConfigDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "EquipmentSlotConfigDataAsset.generated.h"

USTRUCT(BlueprintType)
struct SHOOTERGAME_API FEquipmentSlotUIConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	EEquipmentSlot EquipmentSlot = EEquipmentSlot::Primary;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FText DisplayName = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	EEquipmentContainerType ContainerType = EEquipmentContainerType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	bool bIsExpandable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FGameplayTag RequiredSlotTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	FGameplayTagContainer AllowedItemTypeTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	int32 SortOrder = 0;
};

UCLASS(BlueprintType)
class SHOOTERGAME_API UEquipmentSlotConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment Slot")
	TArray<FEquipmentSlotUIConfig> SlotConfigs;

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	bool GetSlotConfig(EEquipmentSlot InEquipmentSlot, FEquipmentSlotUIConfig& OutConfig) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment Slot")
	TArray<FEquipmentSlotUIConfig> GetSortedSlotConfigs() const;
};