// Source/ShooterGame/Components/CombatComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/FireMode.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "CombatComponent.generated.h"


// Local-only reticle runtime state, computed each tick on owning client
USTRUCT()
struct FReticleState
{
	GENERATED_BODY()

	bool bIsEquipped         = false;
	bool bIsAiming           = false;
	bool bIsCrouched         = false;
	float SpreadAlpha        = 0.f;
	FVector2D CursorScreenPos = FVector2D(0.f, 0.f);
	bool bCursorValid        = false;
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	friend class AShooterGameCharacter;
	friend class AShooterGamePlayerController;

	void EquipWeapon(class AWeapon* WeaponToEquip);

	// Called by AAmmoPickup — grants a magazine to the inventory
	void PickupMagazine(const FMagazine& Mag);

	// Initiates a reload — validates, calls ServerReload
	void ReloadEquippedWeapon();

	// Returns true if a reload is currently possible
	bool CanReload() const;
	
	void EquipSuppressor();
	void RemoveSuppressor();
	bool CurrentWeaponHasSuppressor() const;
	void ToggleSuppressor();
	
	/**
	 * Called by the progression/perk system to apply an accuracy bonus.
	 * Values below 1.0 tighten spread (improve accuracy).
	 * Values above 1.0 widen spread (penalty).
	 * Default 1.0 = no modification.
	 * Clamped to 0.1 - 2.0 to prevent extreme values.
	 */
	void SetAimAccuracyBonus(float InMultiplier);

	FORCEINLINE const FReticleState& GetReticleState()			const { return ReticleState; }
	FORCEINLINE FVector              GetReticleWorldPosition()	const { return ReticleWorldPosition; }
	FORCEINLINE float				 GetBaseWalkSpeed()			const { return BaseWalkSpeed; }

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void FireButtonPressed(bool bPressed);
	void FireButtonReleased();
	void CycleFireMode();
	void HandleFire();
	void HandleBurstFire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& InHitTarget);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire();

	// -----------------------------------------------------------------------
	// Reload RPCs
	// -----------------------------------------------------------------------

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastReload();

private:

	class AShooterGameCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	bool bFireButtonPressed;
	FVector HitTarget;

	// -----------------------------------------------------------------------
	// Fire Cycle State
	// -----------------------------------------------------------------------
	FTimerHandle BurstFireTimerHandle;
	float LastFireTime    = -1.f;
	int32 BurstShotsRemaining = 0;
	bool bFullAutoFiring  = false;
	bool bFiredThisPress  = false;
	bool bBurstInProgress = false;

	// -----------------------------------------------------------------------
	// Reload State
	// -----------------------------------------------------------------------
	bool bIsReloading = false;
	FTimerHandle ReloadTimerHandle;

	// Duration of the reload — should match your reload montage length.
	// Set this in BP defaults or tune here.
	UPROPERTY(EditAnywhere, Category = "Combat|Reload")
	float ReloadDuration = 2.0f;

	// Called when reload timer completes — actually swaps the magazine
	void FinishReload();

	// -----------------------------------------------------------------------
	// Movement Config
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	// -----------------------------------------------------------------------
	// Reticle State (local client only, never replicated)
	// -----------------------------------------------------------------------
	FVector ReticleWorldPosition = FVector::ZeroVector;
	FReticleState ReticleState;

	void UpdateReticleState();
	void UpdateReticleWorldPosition();
	float GetActiveFireRate() const;

	UPROPERTY(EditAnywhere, Category = "Reticle")
	float ReticleMaxDistance = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Reticle")
	float CrouchSpreadMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Reticle")
	float MovementSpreadMultiplier = 0.4f;
	
	UPROPERTY(EditAnywhere, Category = "Reticle")
	float ProjectileFireHeightOffset = 50.f;
	
	// Accuracy multiplier set by the progression system.
	// Applied to weapon spread when aiming. Default = 1.0 (no change).
	float AimAccuracyBonusMultiplier = 1.0f;
	
	void ApplyDownedDebuffsPreFire();
};