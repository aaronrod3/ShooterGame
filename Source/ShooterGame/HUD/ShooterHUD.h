// Source/ShooterGame/HUD/ShooterHUD.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/HUD/Widgets/HUDWidget.h"
#include "ShooterHUD.generated.h"

class AShooterGamePlayerController;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	/**
	 * Binds this HUD to a weapon's OnAmmoChanged delegate.
	 * Call this whenever the player equips a new weapon.
	 * Unbinds from the previous weapon automatically.
	 */
	void BindToWeapon(AWeapon* NewWeapon);

private:
	void DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config);
	void DrawCursorCircle(const FVector2D& Center, const FReticleConfig& Config);

	UPROPERTY(EditAnywhere, Category = "HUD|Reticle")
	FReticleConfig UnequippedReticle;

	// The HUD widget instance — created at BeginPlay
	UPROPERTY()
	UHUDWidget* HUDWidget = nullptr;

	// Widget class to instantiate — set this in the HUD Blueprint defaults
	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widgets")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	// Tracks the currently bound weapon so we can unbind before switching
	UPROPERTY()
	AWeapon* BoundWeapon = nullptr;

	// Called by OnAmmoChanged delegate — routes to widget
	UFUNCTION()
	void OnAmmoChanged(int32 MagRounds, int32 MagCapacity);
};