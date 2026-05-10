// Source/ShooterGame/Interaction/LootContainerActor.cpp
#include "Interaction/LootContainerActor.h"

#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/Character/ShooterGameCharacter.h"

ALootContainerActor::ALootContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	LootInventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("LootInventory"));
}

void ALootContainerActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && LootInventory)
	{
		LootInventory->OnInventoryChanged.AddDynamic(this, &ALootContainerActor::HandleLootInventoryChanged);
	}
}

void ALootContainerActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALootContainerActor, bIsLooted);
}

void ALootContainerActor::Highlight_Implementation()
{
	BP_OnHighlighted();
}

void ALootContainerActor::UnHighlight_Implementation()
{
	BP_OnUnHighlighted();
}

void ALootContainerActor::Interact_Implementation(ACharacter* InstigatingCharacter)
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

	OnLootContainerInteracted.Broadcast(this, ShooterCharacter);
	BP_OnInteracted(ShooterCharacter);
}

bool ALootContainerActor::CanInteract_Implementation(ACharacter* InstigatingCharacter) const
{
	return !bIsLooted && LootInventory && LootInventory->GetItemCount() > 0;
}

FText ALootContainerActor::GetInteractPromptText_Implementation(ACharacter* InstigatingCharacter) const
{
	return InteractPromptText;
}

void ALootContainerActor::HandleLootInventoryChanged()
{
	if (!HasAuthority())
	{
		return;
	}

	if (LootInventory && LootInventory->GetItemCount() == 0)
	{
		bIsLooted = true;
		BP_OnLooted();
	}
}

void ALootContainerActor::OnRep_bIsLooted()
{
	if (bIsLooted)
	{
		BP_OnLooted();
	}
}