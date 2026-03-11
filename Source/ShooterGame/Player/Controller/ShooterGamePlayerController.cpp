// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterGamePlayerController.h"
#include "Items/Components/ItemComponent.h"
#include "ShooterGame/HUD/Widgets/HUDWidget.h"
#include "Inventory/InventoryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Interaction/Highlightable.h"
#include "Kismet/GameplayStatics.h"
#include "InputMappingContext.h"

#include "Blueprint/UserWidget.h"
#include "Interaction/HighlightableStaticMesh.h"


AShooterGamePlayerController::AShooterGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	TraceLength = 500.f;
	ItemTraceChannel = ECC_GameTraceChannel1;
}

void AShooterGamePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TraceForItem();
}


void AShooterGamePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Show cursor
	bShowMouseCursor = true;

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}

	}
	
	InventoryComponent = FindComponentByClass<UInventoryComponent>();
	
	CreateHUDWidget();
}

void AShooterGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &AShooterGamePlayerController::PrimaryInteract);
	
}

void AShooterGamePlayerController::PrimaryInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Primary Interact"));
}

void AShooterGamePlayerController::CreateHUDWidget()
{
	if (!IsLocalPlayerController()) return;
	HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
	if (IsValid(HUDWidget))
	{
		HUDWidget->AddToViewport();
	}
}



void AShooterGamePlayerController::TraceForItem()
{
	if (!IsValid(GEngine) || !IsValid(GEngine->GameViewport)) return;
	FVector2D ViewportSize;
	GEngine->GameViewport->GetViewportSize(ViewportSize);
	const FVector2D ViewportCenter = ViewportSize / 2.f;
	FVector TraceStart;
	FVector Forward;
	if (!UGameplayStatics::DeprojectScreenToWorld(this, ViewportCenter, TraceStart, Forward)) return;

	const FVector TraceEnd = TraceStart + Forward * TraceLength;
	FHitResult HitResult;
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ItemTraceChannel);

	LastActor = ThisActor;
	ThisActor = HitResult.GetActor();
	
	if (!ThisActor.IsValid())
	{
		if (IsValid(HUDWidget)) HUDWidget->HidePickupMessage();
	}

	if (ThisActor == LastActor) return;

	if (ThisActor.IsValid())
	{
		if (UActorComponent* Highlightable = ThisActor->FindComponentByInterface(UHighlightable::StaticClass()); IsValid(Highlightable))
		{
			IHighlightable::Execute_Highlight(Highlightable);
		}
		
		
		UItemComponent* ItemComponent = ThisActor->FindComponentByClass<UItemComponent>();
		if (!IsValid(ItemComponent)) return;

		if (IsValid(HUDWidget)) HUDWidget->ShowPickupMessage(ItemComponent->GetPickupMessage());
	}

	if (LastActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Stopped tracing last actor."))
		if (UActorComponent* Highlightable = LastActor->FindComponentByInterface(UHighlightable::StaticClass()); IsValid(Highlightable))
		{
			IHighlightable::Execute_UnHighlight(Highlightable);
		}
	}
}





