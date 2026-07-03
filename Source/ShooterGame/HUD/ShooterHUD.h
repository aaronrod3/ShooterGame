// Source/ShooterGame/HUD/ShooterHUD.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/HUD/Widgets/HUDWidget.h"
#include "ShooterHUD.generated.h"

class AShooterGamePlayerController;
class ALootContainerActor;
class ASquadCacheActor;
class AShooterGameCharacter;
class UStashWindowWidget;
class UEquipmentPanelWidget;
class ULootContainerWidget;
class USquadCacheWidget;
class UPostExtractionWidget;
class UQuickSlotBarWidget;
class AVendorNPCActor;
class UVendorWidget;
class UQuestbookWidget;

UCLASS()
class SHOOTERGAME_API AShooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	// ── Weapon binding ──────────────────────────────────────────────────────
	void BindToWeapon(AWeapon* NewWeapon);

protected:
	virtual void BeginPlay() override;

private:
	// ── Crosshair / reticle ─────────────────────────────────────────────────
	void DrawCrosshairReticle(const FVector2D& Center, const FReticleState& State, const FReticleConfig& Config);

	UPROPERTY(EditAnywhere, Category = "HUD|Reticle")
	FReticleConfig UnequippedReticle;

	// ── HUD widget ──────────────────────────────────────────────────────────
	UPROPERTY()
	UHUDWidget* HUDWidget = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "HUD|Widgets")
	TSubclassOf<UHUDWidget> HUDWidgetClass;

	// ── Ammo binding ────────────────────────────────────────────────────────
	UPROPERTY()
	AWeapon* BoundWeapon = nullptr;

	UFUNCTION()
	void OnAmmoChanged(int32 MagRounds, int32 MagCapacity);

	

	AShooterGameCharacter* GetOwningShooterCharacter() const;
};