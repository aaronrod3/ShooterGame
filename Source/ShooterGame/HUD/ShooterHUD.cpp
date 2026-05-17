// Source/ShooterGame/HUD/ShooterHUD.cpp
#include "HUD/ShooterHUD.h"

#include "Blueprint/UserWidget.h"
#include "Player/Character/ShooterGameCharacter.h"
#include "EngineUtils.h"
#include "Player/Controller/ShooterGamePlayerController.h"

void AShooterHUD::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Create the persistent HUD widget (ammo, crosshair, etc.)
	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UHUDWidget>(PC, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport(0);
		}
	}
	
	
}

// ── Existing weapon/ammo binding ────────────────────────────────────────────

void AShooterHUD::BindToWeapon(AWeapon* NewWeapon)
{
	if (BoundWeapon)
	{
		BoundWeapon->OnAmmoChanged.RemoveDynamic(this, &AShooterHUD::OnAmmoChanged);
	}

	BoundWeapon = NewWeapon;

	if (BoundWeapon)
	{
		BoundWeapon->OnAmmoChanged.AddDynamic(this, &AShooterHUD::OnAmmoChanged);
	}
}

void AShooterHUD::OnAmmoChanged(int32 MagRounds, int32 MagCapacity)
{
	if (HUDWidget)
	{
		HUDWidget->UpdateAmmoDisplay(MagRounds, MagCapacity);
	}
}

// ── Helpers ──────────────────────────────────────────────────────────────────

AShooterGameCharacter* AShooterHUD::GetOwningShooterCharacter() const
{
	if (APlayerController* PC = GetOwningPlayerController())
	{
		return Cast<AShooterGameCharacter>(PC->GetPawn());
	}

	return nullptr;
}


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

		// TPS — reticle is always locked to screen center, never follows mouse
		int32 SizeX, SizeY;
		ShooterGamePlayerController->GetViewportSize(SizeX, SizeY);
		DrawCrosshairReticle(FVector2D(SizeX * 0.5f, SizeY * 0.5f), State, Config);
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
	
}

