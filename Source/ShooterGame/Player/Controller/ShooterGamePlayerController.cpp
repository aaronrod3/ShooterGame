// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterGamePlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "ShooterGame/HUD/Widgets/HUDWidget.h"
#include "Blueprint/UserWidget.h"

void AShooterGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Contexts
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* CurrentContext : DefaultIMCs)
		{
			Subsystem->AddMappingContext(CurrentContext, 0);
		}

	}
	
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








