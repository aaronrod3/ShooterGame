// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterHUD.generated.h"


class AShooterGamePlayerController;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void DrawHUD() override;
	
private:
	void DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config);
	void DrawCursorCircle(const FVector2D& Center, const FReticleConfig& Config);
	
	UPROPERTY(EditAnywhere, Category = "HUD | Reticle")
	FReticleConfig UnequippedReticle;
};
