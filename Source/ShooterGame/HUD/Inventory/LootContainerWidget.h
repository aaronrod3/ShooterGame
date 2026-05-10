// Source/ShooterGame/HUD/Inventory/LootContainerWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LootContainerWidget.generated.h"

class ALootContainerActor;
class AShooterGameCharacter;
class USubContainerWidget;

UCLASS()
class SHOOTERGAME_API ULootContainerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Loot Container Widget")
	void InitializeLootWindow(ALootContainerActor* InLootActor, AShooterGameCharacter* InPlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Loot Container Widget")
	void CloseLootWindow();

	UFUNCTION(BlueprintPure, Category = "Loot Container Widget")
	ALootContainerActor* GetLootActor() const { return LootActor; }

protected:
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Container Widget")
	void BP_OnLootWindowInitialized(ALootContainerActor* InLootActor, AShooterGameCharacter* InPlayerCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Container Widget")
	void BP_OnLootWindowClosed();

	// Left panel — loot container inventory
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Loot Container Widget")
	TObjectPtr<USubContainerWidget> LootContainerPanel;

	// Right panel — compressed player inventory (no paper doll)
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Loot Container Widget")
	TObjectPtr<USubContainerWidget> PlayerInventoryPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Loot Container Widget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ALootContainerActor> LootActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Loot Container Widget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AShooterGameCharacter> OwningCharacter = nullptr;
};