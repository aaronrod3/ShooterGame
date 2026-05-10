// Source/ShooterGame/Inventory/QuickSlotComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/QuickSlotTypes.h"
#include "QuickSlotComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuickSlotsChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickSlotComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	bool Server_AssignQuickSlot(int32 SlotIndex, EQuickSlotCategory Category, const FGuid& ItemInstanceID);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	bool Server_ClearQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	void Server_ClearAllQuickSlots();

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	bool GetQuickSlotAssignment(int32 SlotIndex, FQuickSlotAssignment& OutAssignment) const;

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	TArray<FQuickSlotAssignment> GetAllQuickSlotAssignments() const;

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	bool IsValidQuickSlotIndex(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	int32 GetMaxQuickSlots() const { return MaxQuickSlots; }

	UPROPERTY(BlueprintAssignable, Category = "Quick Slots|Events")
	FOnQuickSlotsChanged OnQuickSlotsChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_QuickSlotAssignments, VisibleAnywhere, BlueprintReadOnly, Category = "Quick Slots")
	TArray<FQuickSlotAssignment> QuickSlotAssignments;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quick Slots", meta = (ClampMin = "1"))
	int32 MaxQuickSlots = 9;

	UFUNCTION()
	void OnRep_QuickSlotAssignments();

	virtual void BeginPlay() override;

private:
	void InitializeDefaultSlots();
	void BroadcastQuickSlotsChanged();
	FQuickSlotAssignment* FindAssignmentMutable(int32 SlotIndex);
	const FQuickSlotAssignment* FindAssignment(int32 SlotIndex) const;
};