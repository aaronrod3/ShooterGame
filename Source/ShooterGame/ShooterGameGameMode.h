// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AShooterGameGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AShooterGameGameMode();
};



