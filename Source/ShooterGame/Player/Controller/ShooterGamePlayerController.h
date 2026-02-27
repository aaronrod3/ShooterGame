// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterGamePlayerController.generated.h"

class UHUDWidget;
class UInputMappingContext;
class UInputAction;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class AShooterGamePlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	
private:
	
	void PrimaryInteract();
	void CreateHUDWidget();
	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultIMCs;

	
	/*** INPUT ACTIONS***/
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TObjectPtr<UInputAction> PrimaryInteractAction;
	
	
	
	
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;
};
