// Source/ShooterGame/Interaction/SquadCacheActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Highlightable.h"
#include "Interaction/Interactable.h"
#include "SquadCacheActor.generated.h"

class UInventoryComponent;
class AShooterGameCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSquadCacheInteracted,
	ASquadCacheActor*, SquadCache,
	AShooterGameCharacter*, InstigatingCharacter);

UCLASS()
class SHOOTERGAME_API ASquadCacheActor : public AActor, public IHighlightable, public IInteractable
{
	GENERATED_BODY()

public:
	ASquadCacheActor();

	// IHighlightable
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;

	// IInteractable
	virtual void Interact_Implementation(ACharacter* InstigatingCharacter) override;
	virtual bool CanInteract_Implementation(ACharacter* InstigatingCharacter) const override;
	virtual FText GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const override;

	UFUNCTION(BlueprintCallable, Category = "Squad Cache")
	UInventoryComponent* GetCacheInventory() const { return CacheInventory; }

	UPROPERTY(BlueprintAssignable, Category = "Squad Cache|Events")
	FOnSquadCacheInteracted OnSquadCacheInteracted;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Squad Cache")
	void BP_OnHighlighted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Squad Cache")
	void BP_OnUnHighlighted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Squad Cache")
	void BP_OnInteracted(AShooterGameCharacter* InstigatingCharacter);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Squad Cache", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInventoryComponent> CacheInventory;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Squad Cache")
	FText InteractPromptText = FText::FromString(TEXT("Open Cache"));
};