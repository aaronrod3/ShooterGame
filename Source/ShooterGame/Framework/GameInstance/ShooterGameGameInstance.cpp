// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterGameGameInstance.h"
#include "ShooterGame/Player/Controller/ShooterGamePlayerController.h"



DEFINE_LOG_CATEGORY(LogShooterGameGameInstance);


// Initialize Game Instance object
void UShooterGameGameInstance::Init()
{
	// log when game instance is initialized
	UE_LOG(LogShooterGameGameInstance, Log, TEXT("Survival Game Instance Initialized"));
	
	// calls the base USurvivalGameInstance to ensure the engine performs its default initilization
	Super::Init();
}


// Shutdown Game instance object
void UShooterGameGameInstance::Shutdown()
{
	// log when game instance is shutdown
	UE_LOG(LogShooterGameGameInstance, Log, TEXT("Survival Game Instance Shutdown"));
	
	// calls the base USurvivalGameInstance to ensure the engine performs its default shutdown
	Super::Shutdown();
}

// class constructor
UShooterGameGameInstance::UShooterGameGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}


// get primary controller
AShooterGamePlayerController* UShooterGameGameInstance::GetPrimaryPlayerController() const
{
	// safely cast the primary controller from the base type to your custom controller type
	return Cast<AShooterGamePlayerController>(Super::GetPrimaryPlayerController(false));
	
}