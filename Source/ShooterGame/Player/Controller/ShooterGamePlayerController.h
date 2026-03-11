// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ShooterGamePlayerController.generated.h"

class UInventoryComponent;
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
	
	
public:
	AShooterGamePlayerController();
	virtual void Tick(float DeltaTime) override;
	
	
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	
private:
	
	void PrimaryInteract();
	void CreateHUDWidget();
	void TraceForItem();
	
	TWeakObjectPtr<UInventoryComponent> InventoryComponent;
	
	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input")
	TArray<UInputMappingContext*> DefaultIMCs;

	
	/*** INPUT ACTIONS***/
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> PrimaryInteractAction;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TSubclassOf<UHUDWidget> HUDWidgetClass;
	UPROPERTY()
	TObjectPtr<UHUDWidget> HUDWidget;
	
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	double TraceLength;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TEnumAsByte<ECollisionChannel> ItemTraceChannel;
	TWeakObjectPtr<AActor> ThisActor;
	TWeakObjectPtr<AActor> LastActor;
	
};
