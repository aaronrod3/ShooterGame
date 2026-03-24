// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterHUD.h"
#include "Items/Weapon/Weapon.h"
#include "ShooterGame/Player/Controller/ShooterGamePlayerController.h"
#include "Engine/Canvas.h"

void AShooterHUD::DrawHUD()
{
	Super::DrawHUD();

	AShooterGamePlayerController* PlayerController = Cast<AShooterGamePlayerController>(GetOwningPlayerController());
	if (!PlayerController) return;

	if (PlayerController->IsLocalPlayerEquipped())
	{
		const float ReachRadius = GetReachLimitedRadius(PlayerController);
		DrawCircleReticle(PlayerController, ReachRadius);
	}
	else
	{
		DrawDotReticle();
	}
}

void AShooterHUD::DrawDotReticle()
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	const FVector2D Center(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);

	DrawRect(DotReticleColor, Center.X - DotReticleSize / 2.f, Center.Y - DotReticleSize / 2.f,
			 DotReticleSize, DotReticleSize);
}

void AShooterHUD::DrawCircleReticle(const AShooterGamePlayerController* ShooterPlayerController, float OverrideRadius)
{
	if (!Canvas || !ShooterPlayerController) return;

	const float CenterX = Canvas->SizeX * 0.5f;
	const float CenterY = Canvas->SizeY * 0.5f;

	const float MaxSpread = FMath::Max(ShooterPlayerController->GetCurrentWeaponMaxSpread(), 0.001f);
	const float SpreadAlpha = FMath::Clamp(ShooterPlayerController->GetCurrentWeaponSpread() / MaxSpread, 0.f, 1.f);

	// --- Spread + aim radius ---
	float Radius = FMath::Lerp(ReticleBaseRadius, ReticleMaxRadius, SpreadAlpha);

	if (ShooterPlayerController->IsLocalPlayerAiming())
	{
		Radius *= ReticleAimMultiplier;
	}

	// --- Clamp to reach if trace gave us a valid override --- (Step 6 addition)
	if (OverrideRadius >= 0.f)
	{
		Radius = FMath::Min(Radius, OverrideRadius);
	}

	// --- Draw circle ---
	const float AngleStep = (2.f * PI) / FMath::Max(CircleSegments, 3);
	for (int32 Index = 0; Index < CircleSegments; ++Index)
	{
		const float A0 = AngleStep * Index;
		const float A1 = AngleStep * (Index + 1);

		const FVector2D P0(CenterX + Radius * FMath::Cos(A0), CenterY + Radius * FMath::Sin(A0));
		const FVector2D P1(CenterX + Radius * FMath::Cos(A1), CenterY + Radius * FMath::Sin(A1));

		DrawLine(P0.X, P0.Y, P1.X, P1.Y, CircleReticleColor, CircleThickness);
	}
}


float AShooterHUD::GetReachLimitedRadius(const AShooterGamePlayerController* ShooterPlayerController) const
{
	if (!ShooterPlayerController || !Canvas || !GetWorld()) return -1.f;

	const FVector TraceStart = ShooterPlayerController->GetLocalPlayerFlatAimOrigin();
	const FVector AimDir = ShooterPlayerController->GetLocalPlayerFlatAimDirection();
	const float WeaponRange = ShooterPlayerController->GetCurrentWeaponRange();

	if (WeaponRange <= 0.f || AimDir.IsNearlyZero()) return -1.f;

	const FVector TraceEnd = TraceStart + AimDir * WeaponRange;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ReticleReachTrace), false);
	if (const APawn* Pawn = ShooterPlayerController->GetPawn())
	{
		Params.AddIgnoredActor(Pawn);
	}

	GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params);

	const FVector EndPoint = Hit.bBlockingHit ? Hit.ImpactPoint : TraceEnd;

	FVector2D CenterScreen(Canvas->SizeX * 0.5f, Canvas->SizeY * 0.5f);
	FVector2D EndScreen;
	const bool bProjected = ShooterPlayerController->ProjectWorldLocationToScreen(EndPoint, EndScreen, true);
	if (!bProjected) return -1.f;

	return FVector2D::Distance(CenterScreen, EndScreen);
}
