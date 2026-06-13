// Source/ShooterGame/Components/CombatComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShooterGame/Types/FireMode.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "ShooterGame/Items/Weapon/WeaponConfig.h"
#include "ShooterGame/Types/PlayerWeaponStance.h"
#include "ShooterGame/Types/CombatTypes.h"
#include "ShooterGame/Components/LoadoutComponent.h"
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
	
	/** Called by the character to push action state. Used for interactions and future actions. */
	void SetCombatAction(ECombatAction NewAction);
	
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

	FORCEINLINE const FReticleState&	GetReticleState()				const { return ReticleState; }
	FORCEINLINE FVector					GetReticleWorldPosition()		const { return ReticleWorldPosition; }
	FORCEINLINE float					GetBaseWalkSpeed()				const { return BaseWalkSpeed; }
	FORCEINLINE bool					IsReloadPendingLocal()			const { return bLocalReloadPending; }
	FORCEINLINE ECombatAction			GetCombatAction()				const { return CurrentCombatAction; }
	FORCEINLINE EReloadType				GetReloadType()					const { return CurrentReloadType; }
	FORCEINLINE EWeaponGrip				GetCurrentGrip()				const { return CurrentGrip; }
	FORCEINLINE FRotator				GetRecoilRotationTarget()		const { return RecoilRotationTarget; }
	FORCEINLINE FVector					GetRecoilTranslationTarget()	const { return RecoilTranslationTarget; }
	FORCEINLINE float					GetADSAlpha()					const { return ADSAlpha; }
	FORCEINLINE FVector					GetADSLocationTarget()			const { return ADSLocationTarget; }
	FORCEINLINE FRotator				GetADSRotationTarget()			const { return ADSRotationTarget; }
	FORCEINLINE bool					IsBusy()						const { return bIsBusy; }
	FORCEINLINE bool					IsAimingBlocked()				const { return bIsAimingBlocked; }
	FORCEINLINE float					GetAimSpeedMultiplier()			const { return AimSpeedMultiplier; }
	bool								IsReloadAnimationActive()		const;
	EPlayerWeaponStance					GetPlayerWeaponStance()			const;
	
	
	
	/** Current left-hand grip. Drives grip-blend in the anim instance. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	EWeaponGrip CurrentGrip = EWeaponGrip::Default;

	/**
	 * True when any action is running that should block new inputs
	 * (reloading, interacting, inspecting, healing, etc.).
	 * Fire and ADS are gated by this — never set it for Firing itself.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsBusy = false;

	/**
	 * True during an ADS window that the current montage has blocked
	 * (e.g., reload start, equip). Prevents entering ADS mid-animation.
	 * Set/cleared by notify state in Milestone 7.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|State")
	bool bIsAimingBlocked = false;
	
	/** Fraction of current tier speed applied while aiming. Default 0.5 = half speed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Aiming")
	float AimSpeedMultiplier = 0.5f;
	
	/** Current accumulated recoil rotation target. Interpolated toward per-shot kick, decays to zero. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Recoil")
	FRotator RecoilRotationTarget = FRotator::ZeroRotator;

	/** Current accumulated recoil translation target. Interpolated toward per-shot kick, decays to zero. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Recoil")
	FVector RecoilTranslationTarget = FVector::ZeroVector;

	/**
	 * 0.0 = fully hip-fire, 1.0 = fully ADS.
	 * Driven by SetAiming() — used by the AnimGraph to lerp between hip and ADS poses.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS")
	float ADSAlpha = 0.f;

	/** World-space location offset target for ADS. Read from WeaponConfig on equip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS")
	FVector ADSLocationTarget = FVector::ZeroVector;

	/** Rotation offset target for ADS. Read from WeaponConfig on equip. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|ADS")
	FRotator ADSRotationTarget = FRotator::ZeroRotator;
	
	UFUNCTION()
	void OnLoadoutUpdated(const FLoadoutData& NewLoadout);


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
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFinishReload();
	
	
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerPlayDryFire();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerCycleFireMode();

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerToggleSuppressor(bool bEquipping);
	
	

private:

	class AShooterGameCharacter* Character;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_Aiming)
	bool bAiming;
	UFUNCTION()
	void OnRep_Aiming();

	bool bFireButtonPressed;
	FVector HitTarget;
	
	
	/** Weapon actor class to spawn at startup. Assign in character BP defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Starter Weapon")
	TSubclassOf<AWeapon> DefaultWeaponClass;

	/** Config asset to initialize the starter weapon from. Assign in character BP defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Starter Weapon")
	TObjectPtr<UWeaponConfig> DefaultWeaponConfig;

	/** Server-only. Spawns, initializes, and equips the starter weapon. */
	void SpawnDefaultWeapon();
	
	void SpawnAndEquipWeaponFromSlot(const FLoadoutSlot& Slot);
	
	// -----------------------------------------------------------------------
	// Combat Action State (Milestone 4)
	// -----------------------------------------------------------------------

	/** The action the character is currently performing. Server-authoritative. */
	UPROPERTY(ReplicatedUsing = OnRep_CombatAction, VisibleAnywhere, Category = "Combat|State")
	ECombatAction CurrentCombatAction = ECombatAction::None;

	UFUNCTION()
	void OnRep_CombatAction();

	/** Which reload variant is active. Set alongside CurrentCombatAction = Reloading. */
	UPROPERTY(VisibleAnywhere, Category = "Combat|State")
	EReloadType CurrentReloadType = EReloadType::None;

	

	// Internal fire state — these stay local, not replicated
	bool bFullAutoFiring    = false;
	bool bBurstInProgress   = false;

	// Client-local cosmetic gate — cleared by FinishReload, never touches server state
	bool bLocalReloadPending = false;

	// -----------------------------------------------------------------------
	// Fire Cycle State
	// -----------------------------------------------------------------------
	FTimerHandle BurstFireTimerHandle;
	float LastFireTime    = -1.f;
	int32 BurstShotsRemaining = 0;
	bool bFiredThisPress  = false;

	// -----------------------------------------------------------------------
	// Reload State
	// -----------------------------------------------------------------------
	
	FTimerHandle ReloadTimerHandle;
	FTimerHandle ServerReloadTimerHandle;
	void FinishReload_Server();
	

	// Duration of the reload — should match your reload montage length.
	// Set this in BP defaults or tune here.
	UPROPERTY(EditAnywhere, Category = "Combat|Reload")
	float ReloadDuration = 1.0f;

	// Called when reload timer completes — actually swaps the magazine
	void FinishReload();

	// -----------------------------------------------------------------------
	// Movement Config
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;
	
	/** How fast recoil rotation decays back to zero per second. */
	UPROPERTY(EditAnywhere, Category = "Combat|Recoil")
	float RecoilDecaySpeed = 8.f;

	/** How fast ADSAlpha interpolates toward its target (0 or 1). */
	UPROPERTY(EditAnywhere, Category = "Combat|ADS")
	float ADSInterpSpeed = 10.f;

	// -----------------------------------------------------------------------
	// Reticle State (local client only, never replicated)
	// -----------------------------------------------------------------------
	FVector ReticleWorldPosition = FVector::ZeroVector;
	FReticleState ReticleState;

	void UpdateReticleState();
	void UpdateReticleWorldPosition();
	FVector ComputeFinalHitTarget() const;
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
	
	//
	// Aiming
	//
	
	// Delayed hip-fire turn-and-shoot state
	bool bPendingHipFireShot = false;

	UPROPERTY()
	FVector PendingHipFireTarget = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Combat|HipFire Turn")
	float HipFireTurnYawThreshold = 8.f;

	UPROPERTY(EditAnywhere, Category = "Combat|HipFire Turn")
	float HipFireTurnInterpSpeed = 20.f;
	
	void StartPendingHipFireShot(const FVector& InTarget);
	void UpdatePendingHipFireShot(float DeltaTime);
	void ExecutePendingHipFireShot();

	bool ShouldDelayHipFireShot() const;
	float GetHipFireYawDeltaToControl() const;
};