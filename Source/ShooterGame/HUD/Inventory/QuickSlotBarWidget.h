// Source/ShooterGame/HUD/Inventory/QuickSlotBarWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ShooterGame/Types/QuickSlotTypes.h"
#include "QuickSlotBarWidget.generated.h"

class UQuickSlotComponent;

UCLASS()
class SHOOTERGAME_API UQuickSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	void InitializeQuickSlotBar(UQuickSlotComponent* InQuickSlotComponent);

	UFUNCTION(BlueprintCallable, Category = "Quick Slots")
	void RefreshQuickSlotBar();

	UFUNCTION(BlueprintPure, Category = "Quick Slots")
	UQuickSlotComponent* GetQuickSlotComponent() const { return QuickSlotComponent; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleQuickSlotsChanged();

	UFUNCTION(BlueprintImplementableEvent, Category = "Quick Slots")
	void BP_OnQuickSlotBarRefreshed(const TArray<FQuickSlotAssignment>& Assignments);

	UPROPERTY(BlueprintReadOnly, Category = "Quick Slots", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UQuickSlotComponent> QuickSlotComponent = nullptr;
};