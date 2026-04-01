// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterHUD.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "Items/Weapon/Weapon.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Player/Controller/ShooterGamePlayerController.h"
#include "Engine/Canvas.h"


void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	const FVector2D ScreenCenter(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);

	AShooterGamePlayerController* ShooterGamePlayerController = Cast<AShooterGamePlayerController>(PlayerOwner);
	if (!ShooterGamePlayerController) return;
    
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(ShooterGamePlayerController->GetPawn());
	if (!ShooterCharacter) return;
    
	UCombatComponent* Combat = ShooterCharacter->FindComponentByClass<UCombatComponent>();
	if (!Combat) return;
    
	const FReticleState& State = Combat->GetReticleState();

	if (State.bIsEquipped)
	{
		DrawWeaponReticle(ScreenCenter, State, EquippedReticleConfig);
	}
	else
	{
		DrawDotReticle(ScreenCenter, UnequippedReticleConfig);
	}
}


void AShooterHUD::DrawDotReticle(const FVector2D& Center, const FReticleConfig& Config)
{
	if (!Canvas) return;

	const float Radius = Config.CenterDotRadius;
	const int32 Segments = FMath::Max(Config.CenterDotSegments, 3);
	const float AngleStep = (2.f * PI) / Segments;

	for (int32 i = 0; i < Segments; ++i)
	{
		const float A0 = AngleStep * i;
		const float A1 = AngleStep * (i + 1);

		const FVector2D P0(Center.X + Radius * FMath::Cos(A0), Center.Y + Radius * FMath::Sin(A0));
		const FVector2D P1(Center.X + Radius * FMath::Cos(A1), Center.Y + Radius * FMath::Sin(A1));

		DrawLine(P0.X, P0.Y, P1.X, P1.Y, Config.CenterDotColor, Config.Thickness);
	}
}


void AShooterHUD::DrawWeaponReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config)
{
	if (!Canvas) return;

	float Radius = FMath::Lerp(Config.BaseRadius, Config.MaxRadius, State.SpreadAlpha);
	if (State.bIsCrouched) Radius *= Config.CrouchMultiplier;
	if (State.bIsAiming)   Radius *= Config.AimMultiplier;

	const float GapSize = Config.CrosshairGapSize;
	const float Thickness = Config.Thickness;
	const FLinearColor LineColor = Config.LineColor;

	// Top
	DrawLine(Center.X, Center.Y - GapSize, Center.X, Center.Y - Radius, LineColor, Thickness);

	// Bottom
	DrawLine(Center.X, Center.Y + GapSize, Center.X, Center.Y + Radius, LineColor, Thickness);

	// Left
	DrawLine(Center.X - GapSize, Center.Y, Center.X - Radius, Center.Y, LineColor, Thickness);

	// Right
	DrawLine(Center.X + GapSize, Center.Y, Center.X + Radius, Center.Y, LineColor, Thickness);

	// Center dot — circle
	DrawDotReticle(Center, Config);
}





