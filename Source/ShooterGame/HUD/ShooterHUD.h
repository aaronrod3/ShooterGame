// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterHUD.generated.h"


struct FReticleState;
struct FReticleConfig;



class AShooterGamePlayerController;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;
	
private:
	void DrawDotReticle(const FVector2D& Center, const FReticleConfig& Config);
	void DrawCircleReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config);
	
};
