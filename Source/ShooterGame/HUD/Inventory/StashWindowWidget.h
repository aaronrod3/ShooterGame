// Source/ShooterGame/HUD/Inventory/StashWindowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/LoadoutTypes.h"
#include "StashWindowWidget.generated.h"

class UInventoryItemRowWidget;
class UPanelWidget;
class UStashComponent;

UCLASS()
class SHOOTERGAME_API UStashWindowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash")
	void InitializeStashWindow(UStashComponent* InStashComponent);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash")
	void SetActiveCategory(EItemCategory InCategory);

	UFUNCTION(BlueprintCallable, Category = "Inventory|Stash")
	void RefreshStashItems();

	UFUNCTION(BlueprintPure, Category = "Inventory|Stash")
	UStashComponent* GetStashComponent() const { return StashComponent; }

	UFUNCTION(BlueprintPure, Category = "Inventory|Stash")
	EItemCategory GetActiveCategory() const { return ActiveCategory; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleStashChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Stash")
	UPanelWidget* BP_GetStashItemHostPanel() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Stash")
	void BP_OnActiveCategoryChanged(EItemCategory InCategory);

	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|Stash")
	void BP_OnStashRefreshed(int32 ItemCount);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory|Stash", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UInventoryItemRowWidget> ItemRowWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|Stash", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStashComponent> StashComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Stash", meta = (AllowPrivateAccess = "true"))
	EItemCategory ActiveCategory = EItemCategory::Weapon;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInventoryItemRowWidget>> SpawnedStashRows;
};