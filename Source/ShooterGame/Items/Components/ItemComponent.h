// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class SHOOTERGAME_API UItemComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemComponent();
	
	FString GetPickupMessage() const { return PickupMessage; }
	

protected:
	// Called when the game starts
	virtual void BeginPlay() override;



private:

	UPROPERTY(EditAnywhere, Category = "Item | Component")
	FString PickupMessage;
	


};
