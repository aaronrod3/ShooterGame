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

		// ── CHANGED: draw crosshair at mouse position, not world-projected shot point ──
		float MouseX, MouseY;
		if (ShooterGamePlayerController->GetMousePosition(MouseX, MouseY))
		{
			DrawCrosshairReticle(FVector2D(MouseX, MouseY), State, Config);
		}
		else
		{
			// Fallback: viewport center when mouse position unavailable
			int32 SizeX, SizeY;
			ShooterGamePlayerController->GetViewportSize(SizeX, SizeY);
			DrawCrosshairReticle(FVector2D(SizeX * 0.5f, SizeY * 0.5f), State, Config);
		}
	}
}

void AShooterHUD::DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config)
{
	if (!Canvas) return;

	float Radius = FMath::Lerp(Config.BaseRadius, Config.MaxRadius, State.SpreadAlpha);
	if (State.bIsCrouched) Radius *= Config.CrouchMultiplier;
	if (State.bIsAiming)   Radius *= Config.AimMultiplier;

	const float GapSize = Config.CrosshairGapSize;
	const FLinearColor LineColor = Config.LineColor;         // ← now uses Config.LineColor
	const float T = Config.Thickness;

	DrawLine(Center.X, Center.Y - GapSize, Center.X, Center.Y - Radius, LineColor, T);
	DrawLine(Center.X, Center.Y + GapSize, Center.X, Center.Y + Radius, LineColor, T);
	DrawLine(Center.X - GapSize, Center.Y, Center.X - Radius, Center.Y, LineColor, T);
	DrawLine(Center.X + GapSize, Center.Y, Center.X + Radius, Center.Y, LineColor, T);

	
	const float DotHalfSize = Config.CenterDotRadius;      
	DrawRect(
		Config.CenterDotColor,                               
		Center.X - DotHalfSize,
		Center.Y - DotHalfSize,
		DotHalfSize * 2.f,
		DotHalfSize * 2.f
	);
}
