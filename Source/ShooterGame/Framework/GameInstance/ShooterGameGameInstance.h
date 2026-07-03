// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ShooterGameGameInstance.generated.h"

class AShooterGamePlayerController;
class UObject;


DECLARE_LOG_CATEGORY_EXTERN(LogShooterGameGameInstance, Log, All);



UCLASS()
class SHOOTERGAME_API UShooterGameGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	// Called to get initialize game instance object
	explicit UShooterGameGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// called to retrieve the primary player controller
	AShooterGamePlayerController* GetPrimaryPlayerController() const;
	
	
protected:
	// called to initialize game instance on game startup
	virtual void Init() override;
	
	// called to shutdown game instance on game exit
	virtual void Shutdown() override;
	
	
};



	