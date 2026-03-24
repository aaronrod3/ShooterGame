// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterHUD.generated.h"


class AShooterGamePlayerController;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;
	
private:
	void DrawDotReticle();
	void DrawCircleReticle(const AShooterGamePlayerController* ShooterPlayerController, float OverrideRadius = -1.f);
	
	float GetReachLimitedRadius(const AShooterGamePlayerController* ShooterPC) const;
	

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	float DotReticleSize = 4.f;

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	FLinearColor DotReticleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	float ReticleBaseRadius = 20.f;    // px at 0 spread

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	float ReticleMaxRadius = 80.f;     // px at max spread

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	float ReticleAimMultiplier = 0.4f; // circle tightens to this fraction while aiming

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	FLinearColor CircleReticleColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	int32 CircleSegments = 32;
	
	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	float CircleThickness = 1.5f;
	
};
