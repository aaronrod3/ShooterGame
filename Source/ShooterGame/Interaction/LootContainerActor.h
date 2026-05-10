// Source/ShooterGame/Interaction/LootContainerActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Highlightable.h"
#include "Interaction/Interactable.h"
#include "LootContainerActor.generated.h"

class UInventoryComponent;
class AShooterGameCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLootContainerInteracted,
	ALootContainerActor*, LootContainer,
	AShooterGameCharacter*, InstigatingCharacter);

UCLASS()
class SHOOTERGAME_API ALootContainerActor : public AActor, public IHighlightable, public IInteractable
{
	GENERATED_BODY()

public:
	ALootContainerActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IHighlightable
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;

	// IInteractable
	virtual void Interact_Implementation(ACharacter* InstigatingCharacter) override;
	virtual bool CanInteract_Implementation(ACharacter* InstigatingCharacter) const override;
	virtual FText GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const override;

	UFUNCTION(BlueprintCallable, Category = "Loot Container")
	UInventoryComponent* GetLootInventory() const { return LootInventory; }

	UFUNCTION(BlueprintCallable, Category = "Loot Container")
	bool IsLooted() const { return bIsLooted; }

	UPROPERTY(BlueprintAssignable, Category = "Loot Container|Events")
	FOnLootContainerInteracted OnLootContainerInteracted;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Container")
	void BP_OnHighlighted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Container")
	void BP_OnUnHighlighted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Container")
	void BP_OnInteracted(AShooterGameCharacter* InstigatingCharacter);

	UFUNCTION(BlueprintImplementableEvent, Category = "Loot Container")
	void BP_OnLooted();

	UFUNCTION()
	void HandleLootInventoryChanged();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Loot Container", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> LootInventory;

	UPROPERTY(ReplicatedUsing = OnRep_bIsLooted, VisibleAnywhere, BlueprintReadOnly, Category = "Loot Container")
	bool bIsLooted = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot Container")
	FText InteractPromptText = FText::FromString(TEXT("Loot"));

private:
	UFUNCTION()
	void OnRep_bIsLooted();
};