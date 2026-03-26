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

	AShooterGamePlayerController* ShooterGamePlayerController = Cast<AShooterGamePlayerController>(PlayerOwner);
	if (!ShooterGamePlayerController) return;
    
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(ShooterGamePlayerController->GetPawn());
	if (!ShooterCharacter) return;
    
	UCombatComponent* Combat = ShooterCharacter->FindComponentByClass<UCombatComponent>();
	if (!Combat) return;
    
	const FReticleState& State = Combat->GetReticleState();

	if (State.bIsEquipped)
	{
		const AWeapon* Weapon = ShooterCharacter->GetEquippedWeapon();
		if (!Weapon) return;

		const FReticleConfig& Config = Weapon->GetReticleConfig();

		// Reticle crosshair — projected from world shot position
		FVector2D ReticleScreenPos;
		if (ShooterGamePlayerController->ProjectWorldLocationToScreen(
			Combat->GetReticleWorldPosition(),
			ReticleScreenPos,
			true))
		{
			DrawCrosshairReticle(ReticleScreenPos, State, Config);
		}

		// Cursor circle — drawn at current mouse position, fixed size
		float MouseX, MouseY;
		if (ShooterGamePlayerController->GetMousePosition(MouseX, MouseY))
		{
			DrawCursorCircle(FVector2D(MouseX, MouseY), Config);
		}
		else
		{
			// Fallback: viewport center when mouse is captured
			int32 SizeX, SizeY;
			ShooterGamePlayerController->GetViewportSize(SizeX, SizeY);
			DrawCursorCircle(FVector2D(SizeX * 0.5f, SizeY * 0.5f), Config);
		}
	}
}


void AShooterHUD::DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config)
{
	if (!Canvas) return;

	// Spread-driven radius, same logic as old circle
	float Radius = FMath::Lerp(Config.BaseRadius, Config.MaxRadius, State.SpreadAlpha);
	if (State.bIsCrouched) Radius *= Config.CrouchMultiplier;
	if (State.bIsAiming)   Radius *= Config.AimMultiplier;

	const float GapSize = Config.CrosshairGapSize;
	const FLinearColor Color = Config.CircleColor;
	const float T = Config.Thickness;

	// Top
	DrawLine(Center.X, Center.Y - GapSize, Center.X, Center.Y - Radius, Color, T);
	// Bottom
	DrawLine(Center.X, Center.Y + GapSize, Center.X, Center.Y + Radius, Color, T);
	// Left
	DrawLine(Center.X - GapSize, Center.Y, Center.X - Radius, Center.Y, Color, T);
	// Right
	DrawLine(Center.X + GapSize, Center.Y, Center.X + Radius, Center.Y, Color, T);
	
	// center dot
	DrawRect(
		Config.CenterDotColor,
		Center.X - Config.CenterDotSize * 0.5f,
		Center.Y - Config.CenterDotSize * 0.5f,
		Config.CenterDotSize,
		Config.CenterDotSize
	);
}

void AShooterHUD::DrawCursorCircle(const FVector2D& Center, const FReticleConfig& Config)
{
	if (!Canvas) return;

	const float Radius = Config.CursorCircleRadius; // fixed, never changes
	const int32 Segments = Config.Segments;
	const float AngleStep = (2.f * PI) / FMath::Max(Segments, 3);

	for (int32 Index = 0; Index < Segments; ++Index)
	{
		const float A0 = AngleStep * Index;
		const float A1 = AngleStep * (Index + 1);

		const FVector2D P0(Center.X + Radius * FMath::Cos(A0), Center.Y + Radius * FMath::Sin(A0));
		const FVector2D P1(Center.X + Radius * FMath::Cos(A1), Center.Y + Radius * FMath::Sin(A1));

		DrawLine(P0.X, P0.Y, P1.X, P1.Y, Config.CursorCircleColor, Config.CursorCircleThickness);
	}
}






