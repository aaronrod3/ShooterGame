// Source/ShooterGame/Interaction/Interactable.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

class ACharacter;

UINTERFACE()
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class SHOOTERGAME_API IInteractable
{
	GENERATED_BODY()

public:
	// Called when a player walks up and presses the interact key.
	// Implementors open their relevant UI or trigger their world action.
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Interact(ACharacter* InstigatingCharacter);

	// Returns true if the actor is currently interactable by the given character.
	// Use this to gate range, squad membership, door state, etc.
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(ACharacter* InstigatingCharacter) const;

	// Returns a display string for the interaction prompt (e.g. "Open Cache", "Loot Body").
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	FText GetInteractPromptText(ACharacter* InstigatingCharacter) const;
};