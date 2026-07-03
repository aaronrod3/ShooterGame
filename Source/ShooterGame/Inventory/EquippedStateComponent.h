// Source/ShooterGame/Inventory/EquippedStateComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryComponent.h"
#include "ShooterGame/Types/ContainerTypes.h"
#include "EquippedStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquippedStateChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UEquippedStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEquippedStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	UPROPERTY(BlueprintAssignable, Category = "Equipment|Events")
	FOnEquippedStateChanged OnEquippedStateChanged;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool Server_EquipItem(const FGuid& InstanceID, EEquipmentSlot Slot, UInventoryComponent* SourceContainer);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool Server_UnequipItem(EEquipmentSlot Slot, UInventoryComponent* TargetContainer);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool Server_LootDeadPlayer(UEquippedStateComponent* DeadPlayerEquipment);

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	void RecalculateWeight();
	
	const FItemInstance* GetEquippedItem(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	bool HasEquippedItem(EEquipmentSlot Slot) const;

	FORCEINLINE UInventoryComponent* GetRigInventory() const { return RigInventory; }
	FORCEINLINE UInventoryComponent* GetBeltInventory() const { return BeltInventory; }
	FORCEINLINE UInventoryComponent* GetBackpackInventory() const { return BackpackInventory; }

	FORCEINLINE float GetCurrentCarriedWeight() const { return CurrentCarriedWeight; }
	FORCEINLINE float GetCurrentBurdenScore() const { return CurrentBurdenScore; }
	FORCEINLINE float GetDeadPlayerBurdenMultiplier() const { return DeadPlayerBurdenMultiplier; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_PrimaryWeapon, meta = (AllowPrivateAccess = "true"))
	FItemInstance PrimaryWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_SecondaryWeapon, meta = (AllowPrivateAccess = "true"))
	FItemInstance SecondaryWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_SidearmWeapon, meta = (AllowPrivateAccess = "true"))
	FItemInstance SidearmWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_HelmetItem, meta = (AllowPrivateAccess = "true"))
	FItemInstance HelmetItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_ChestItem, meta = (AllowPrivateAccess = "true"))
	FItemInstance ChestItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_ToolItem, meta = (AllowPrivateAccess = "true"))
	FItemInstance ToolItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", ReplicatedUsing = OnRep_KnifeItem, meta = (AllowPrivateAccess = "true"))
	FItemInstance KnifeItem;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Containers", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> RigInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Containers", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> BeltInventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Containers", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> BackpackInventory;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentCarriedWeight, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weight", meta = (AllowPrivateAccess = "true"))
	float CurrentCarriedWeight = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentBurdenScore, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weight", meta = (AllowPrivateAccess = "true"))
	float CurrentBurdenScore = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DeadPlayerBurdenMultiplier = 1.0f;

private:
	FItemInstance* GetEquippedItemMutable(EEquipmentSlot Slot);
	const FItemInstance* GetEquippedItemInternal(EEquipmentSlot Slot) const;

	static bool IsValidEquipmentSlot(EEquipmentSlot Slot);
	static bool IsSingleItemSlot(EEquipmentSlot Slot);
	static float GetItemWeight(const FItemInstance& Item);
	void BroadcastEquippedStateChanged();

	UFUNCTION()
	void OnRep_PrimaryWeapon();

	UFUNCTION()
	void OnRep_SecondaryWeapon();

	UFUNCTION()
	void OnRep_SidearmWeapon();

	UFUNCTION()
	void OnRep_HelmetItem();

	UFUNCTION()
	void OnRep_ChestItem();

	UFUNCTION()
	void OnRep_ToolItem();

	UFUNCTION()
	void OnRep_KnifeItem();

	UFUNCTION()
	void OnRep_CurrentCarriedWeight();

	UFUNCTION()
	void OnRep_CurrentBurdenScore();
};