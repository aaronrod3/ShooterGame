// Source/ShooterGame/Interaction/TestInteractableActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "Interaction/Highlightable.h"
#include "TestInteractableActor.generated.h"

class UStaticMeshComponent;
class ACharacter;

UCLASS()
class SHOOTERGAME_API ATestInteractableActor : public AActor, public IInteractable, public IHighlightable
{
	GENERATED_BODY()

public:
	ATestInteractableActor();

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Displayed in the interact prompt
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText PromptText = FText::FromString(TEXT("Interact"));

	// Lets designers toggle interactability in-editor or at runtime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bCanCurrentlyInteract = true;

	// Custom depth stencil value — must match your post-process outline material
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction | Highlight")
	int32 HighlightStencilValue = 1;

public:

	// IInteractable
	virtual void Interact_Implementation(ACharacter* InstigatingCharacter) override;
	virtual bool CanInteract_Implementation(ACharacter* InstigatingCharacter) const override;
	virtual FText GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const override;

	// IHighlightable
	virtual void Highlight_Implementation() override;
	virtual void UnHighlight_Implementation() override;
};