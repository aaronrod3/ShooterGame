// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterHUD.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "Items/Weapon/Weapon.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "Engine/Canvas.h"


void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController) return;
	
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(PlayerController->GetPawn());
	if (!ShooterCharacter) return;
	
	UCombatComponent* Combat = ShooterCharacter->FindComponentByClass<UCombatComponent>();
	if (!Combat) return;
	
	
	const FReticleState& State = Combat->GetReticleState();
	
	const FVector2D DrawCenter = State.CursorScreenPos;
	
	if (State.bIsEquipped)
	{
		// Fetch per-weapon style config and draw the circle reticle
		const AWeapon* Weapon = ShooterCharacter->GetEquippedWeapon();
		if (!Weapon) return;
		
		DrawCircleReticle(DrawCenter, State, Weapon->GetReticleConfig());
	}
	else
	{
		// No Weapon - draw the fallback dot using a default config
		DrawDotReticle(DrawCenter, UnequippedReticle);
	}
}



void AShooterHUD::DrawDotReticle(const FVector2D& Center, const FReticleConfig& Config)
{
	DrawRect(
	Config.DotColor,
	Center.X - Config.DotSize * 0.5f,
	Center.Y - Config.DotSize * 0.5f,
	Config.DotSize,
	Config.DotSize	
	);
}

void AShooterHUD::DrawCircleReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config)
{
	if (!Canvas) return;

	// Lerp radius from base to max using pre-normalized spread alpha
	float Radius = FMath::Lerp(Config.BaseRadius, Config.MaxRadius, State.SpreadAlpha);
	
	// Tighten base radius when crouched
	if (State.bIsCrouched)
	{
		Radius *= Config.CrouchMultiplier;
	}

	// Tighten radius while aiming
	if (State.bIsAiming)
	{
		Radius *= Config.AimMultiplier;
	}

	// Clamp to reach radius if a valid surface was hit within weapon range
	if (State.ReachRadius > 1.f)
	{
		Radius = FMath::Min(Radius, State.ReachRadius);
	}

	// Draw segmented circle
	const float AngleStep = (2.f * PI) / FMath::Max(Config.Segments, 3);
	for (int32 Index = 0; Index < Config.Segments; ++Index)
	{
		const float A0 = AngleStep * Index;
		const float A1 = AngleStep * (Index + 1);

		const FVector2D P0(Center.X + Radius * FMath::Cos(A0), Center.Y + Radius * FMath::Sin(A0));
		const FVector2D P1(Center.X + Radius * FMath::Cos(A1), Center.Y + Radius * FMath::Sin(A1));

		DrawLine(P0.X, P0.Y, P1.X, P1.Y, Config.CircleColor, Config.Thickness);
	}
}








