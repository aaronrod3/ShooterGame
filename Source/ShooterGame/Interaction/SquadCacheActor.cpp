// Source/ShooterGame/Interaction/SquadCacheActor.cpp
#include "Interaction/SquadCacheActor.h"

#include "Inventory/InventoryComponent.h"
#include "Player/Character/ShooterGameCharacter.h"

ASquadCacheActor::ASquadCacheActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CacheInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("CacheInventory"));
}

void ASquadCacheActor::BeginPlay()
{
	Super::BeginPlay();
}

void ASquadCacheActor::Highlight_Implementation()
{
	BP_OnHighlighted();
}

void ASquadCacheActor::UnHighlight_Implementation()
{
	BP_OnUnHighlighted();
}

void ASquadCacheActor::Interact_Implementation(ACharacter* InstigatingCharacter)
{
	if (!HasAuthority())
	{
		return;
	}

	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(InstigatingCharacter);
	if (!ShooterCharacter)
	{
		return;
	}

	OnSquadCacheInteracted.Broadcast(this, ShooterCharacter);
	BP_OnInteracted(ShooterCharacter);
}

bool ASquadCacheActor::CanInteract_Implementation(ACharacter* InstigatingCharacter) const
{
	return CacheInventory != nullptr;
}

FText ASquadCacheActor::GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const
{
	return InteractPromptText;
}