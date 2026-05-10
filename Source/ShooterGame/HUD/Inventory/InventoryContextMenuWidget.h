// Source/ShooterGame/HUD/Inventory/ItemContextMenuWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/ItemContextMenuTypes.h"
#include "InventoryContextMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemContextActionSelected, EItemContextAction, SelectedAction);

UCLASS()
class SHOOTERGAME_API UInventoryContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Item Context Menu")
	void InitializeContextMenu(const FItemContextMenuData& InMenuData);

	UFUNCTION(BlueprintCallable, Category = "Item Context Menu")
	void SelectContextAction(EItemContextAction SelectedAction);

	UFUNCTION(BlueprintPure, Category = "Item Context Menu")
	const FItemContextMenuData& GetMenuData() const { return MenuData; }

	UPROPERTY(BlueprintAssignable, Category = "Item Context Menu|Events")
	FOnItemContextActionSelected OnItemContextActionSelected;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Item Context Menu")
	void BP_OnContextMenuInitialized(const FItemContextMenuData& InMenuData);

	UPROPERTY(BlueprintReadOnly, Category = "Item Context Menu", meta = (AllowPrivateAccess = "true"))
	FItemContextMenuData MenuData;
};