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
	const FLinearColor Color = FLinearColor::White;
	const float T = Config.Thickness;

	DrawLine(Center.X, Center.Y - GapSize, Center.X, Center.Y - Radius, Color, T);
	DrawLine(Center.X, Center.Y + GapSize, Center.X, Center.Y + Radius, Color, T);
	DrawLine(Center.X - GapSize, Center.Y, Center.X - Radius, Center.Y, Color, T);
	DrawLine(Center.X + GapSize, Center.Y, Center.X + Radius, Center.Y, Color, T);

	const float CenterDotSize = 4.f;								// ← ADDED: temporary local size, Config.CenterDotSize doesn't exist
	const FLinearColor CenterDotColor = FLinearColor::White;		// ← ADDED: temporary local color, avoids missing config fields

	DrawRect(
		CenterDotColor,
		Center.X - CenterDotSize * 0.5f,
		Center.Y - CenterDotSize * 0.5f,
		CenterDotSize,
		CenterDotSize
	);
}
