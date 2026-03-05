// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"
#include "ShooterGame/HUD/Inventory/InventoryBase.h"


// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	ConstructInventory();
	
}


void UInventoryComponent::ConstructInventory()
{
	OwningController = Cast<APlayerController>(GetOwner());
	checkf(OwningController.IsValid(), TEXT("Inventory Component should have a Player Controller as Owner."))
	if (!OwningController->IsLocalController()) return;

	InventoryMenu = CreateWidget<UInventoryBase>(OwningController.Get(), InventoryMenuClass);
	InventoryMenu->AddToViewport();
} 















