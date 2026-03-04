// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD | Pickup")
	void ShowPickupMessage(const FString& Message);
	
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD | Pickup")
	void HidePickupMessage();
	
	
	
};
