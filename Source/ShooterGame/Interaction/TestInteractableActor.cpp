// Source/ShooterGame/Interaction/TestInteractableActor.cpp
#include "Interaction/TestInteractableActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Engine/Engine.h"

ATestInteractableActor::ATestInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true; // World state changes from Interact must be server-authoritative

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Custom depth off by default — Highlight_Implementation enables it
	Mesh->SetRenderCustomDepth(false);
	Mesh->SetCustomDepthStencilValue(HighlightStencilValue);
}

// --- IInteractable ---

void ATestInteractableActor::Interact_Implementation(ACharacter* InstigatingCharacter)
{
	// This runs on the server because ServerPrimaryInteract_Implementation calls it.
	// Safe to do authoritative world changes here in future phases.
	const FString Name = IsValid(InstigatingCharacter)
		? InstigatingCharacter->GetName() : TEXT("Unknown");

	UE_LOG(LogTemp, Warning,
		TEXT("[TestInteractableActor] Interacted by: %s"), *Name);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 3.f, FColor::Yellow,
			FString::Printf(TEXT("Interacted: %s"), *Name)
		);
	}
}

bool ATestInteractableActor::CanInteract_Implementation(ACharacter* InstigatingCharacter) const
{
	return bCanCurrentlyInteract;
}

FText ATestInteractableActor::GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const
{
	return PromptText;
}

// --- IHighlightable ---

void ATestInteractableActor::Highlight_Implementation()
{
	if (Mesh)
	{
		Mesh->SetCustomDepthStencilValue(HighlightStencilValue);
		Mesh->SetRenderCustomDepth(true);
	}
}

void ATestInteractableActor::UnHighlight_Implementation()
{
	if (Mesh)
	{
		Mesh->SetRenderCustomDepth(false);
	}
}