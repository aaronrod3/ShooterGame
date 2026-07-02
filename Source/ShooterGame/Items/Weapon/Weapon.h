// Source/ShooterGame/Items/Weapon/Weapon.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Types/FireMode.h"
#include "ShooterGame/Types/AmmoType.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "ShooterGame/Items/Ammo/AmmoData.h"
#include "ShooterGame/Items/Weapon/WeaponConfig.h"
#include "ShooterGame/Components/WeaponAudioComponent.h"
#include "ShooterGame/Components/AudioPerceptionComponent.h"
#include "Weapon.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, MagRounds, int32, MagCapacity);
// Fired on server and all clients whenever the suppressor state changes.
// Bind in weapon Blueprints to toggle suppressor mesh visibility.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSuppressorChanged, bool, bEquipped);

class USphereComponent;

// -----------------------------------------------------------------------
// FReticleConfig
// -----------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FReticleConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float BaseRadius = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float MaxRadius = 42.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float CrosshairGapSize = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float Thickness = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float AimMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float CrouchMultiplier = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	float CenterDotRadius = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	int32 CenterDotSegments = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	FLinearColor CenterDotColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reticle")
	FLinearColor LineColor = FLinearColor::White;
};


// -----------------------------------------------------------------------
// EWeaponState
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial		UMETA(DisplayName = "Initial"),
	EWS_Equipped	UMETA(DisplayName = "Equipped"),
	EWS_Dropped		UMETA(DisplayName = "Dropped"),

	EWS_Max			UMETA(DisplayName = "DefaultMax")
};


// -----------------------------------------------------------------------
// AWeapon
// -----------------------------------------------------------------------
UCLASS()
class SHOOTERGAME_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Broadcasts whenever mag rounds or chamber state changes.
	// Bound by AShooterHUD to update the ammo counter widget.
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Ammo")
	FOnAmmoChanged OnAmmoChanged;

	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	void StopFire();
	void SetWeaponState(EWeaponState State);
	void ApplySpreadMultiplier(float Multiplier);
	
	UFUNCTION(BlueprintPure, Category = "Weapon|Audio")
	bool IsPistolClass() const { return bIsPistolClass; }
	
	UPROPERTY(BlueprintAssignable, Category = "Weapon|Attachments")
	FOnSuppressorChanged OnSuppressorChanged;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Attachments")
	void SetSuppressor(bool bEquipped);

	UFUNCTION(BlueprintPure, Category = "Weapon|Attachments")
	bool HasSuppressor() const { return bHasSuppressor; }
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	UWeaponAudioComponent* WeaponAudioComp;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Audio")
	UAudioPerceptionComponent* AudioPerceptionComp;

	// -----------------------------------------------------------------------
	// Ammo / Feed — public interface
	// -----------------------------------------------------------------------

	/**
	 * Returns true if the weapon has at least one round available to fire.
	 * Checks chamber first, then the inserted magazine.
	 */
	bool CanFire() const;

	/**
	 * Consumes one round. For chambered weapons: fires from chamber,
	 * auto-feeds next round from magazine. For non-chambered (cylinder,
	 * single-shot): consumes directly from the inserted magazine.
	 * Returns true if a round was consumed.
	 */
	bool ConsumeRound();

	/**
	 * Inserts a magazine into the weapon. Rejected if caliber doesn't match
	 * SupportedAmmoType. Used by UCombatComponent during reload.
	 */
	void InsertMagazine(const FMagazine& NewMag);

	/**
	 * Ejects the currently inserted magazine and returns it.
	 * Returns an empty default FMagazine if nothing was inserted.
	 * Used by UCombatComponent at the start of a reload.
	 */
	FMagazine EjectMagazine();

	/**
	 * Attempts to move one round from the inserted magazine into the chamber.
	 * No-op if bSupportsChamberedRound is false, chamber is already loaded,
	 * or the magazine is empty.
	 */
	void ChamberRound();

	// -----------------------------------------------------------------------
	// Notify-driven stubs (infima_integration_plan.md Section 3) — bodies not
	// yet implemented, log a warning until a design pass wires them up.
	// -----------------------------------------------------------------------

	/** Called by AN_DropMagazine at the release frame of a reload montage. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Magazine")
	virtual void SpawnDroppedMagazine(float ImpulseForce, float RotationForce);

	/** Called by AN_EjectCasing at the case-eject frame of a fire montage. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Magazine")
	virtual void EjectCasing(FRotator RotationOffset, float MinEjectForce, float MaxEjectForce, float RotationSpeed);

	/** Called by ANS_HideMainMag / ANS_ShowReserveMag to toggle magazine mesh visibility. */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Magazine")
	virtual void SetMagazineVisibility(bool bVisible, bool bReserve);

	// -----------------------------------------------------------------------
	// Accessors
	// -----------------------------------------------------------------------

	FORCEINLINE USphereComponent*		GetAreaSphere()						const { return CollisionSphere; }
	FORCEINLINE USkeletalMeshComponent*	GetWeaponMesh()						const { return WeaponMesh; }
	FORCEINLINE float					GetCurrentSpread()					const { return CurrentSpread; }
	FORCEINLINE float					GetMaxSpread()						const { return MaxSpread; }
	FORCEINLINE float					GetWeaponRange()					const { return WeaponRange; }
	FORCEINLINE const FReticleConfig&	GetReticleConfig()					const { return ReticleConfig; }
	FORCEINLINE EFireMode				GetCurrentFireMode()				const { return CurrentFireMode; }
	FORCEINLINE float					GetFireRate()						const { return FireRate; }
	FORCEINLINE float					GetFullAutoFireRate()				const { return FullAutoFireRate; }
	FORCEINLINE float					GetBurstFireRate()					const { return BurstFireRate; }
	FORCEINLINE int32					GetBurstCount()						const { return BurstCount; }
	FORCEINLINE bool					IsFireModeAllowed(EFireMode Mode)	const { return AllowedFireModes.Contains(Mode); }
	
	

	// Ammo type this weapon accepts — magazines must match to be inserted
	FORCEINLINE EAmmoType				GetSupportedAmmoType()		const { return SupportedAmmoType; }

	// How this weapon is fed
	FORCEINLINE EWeaponFeedType			GetFeedType()				const { return FeedType; }

	// Whether a round is currently in the chamber
	FORCEINLINE bool					HasChamberedRound()			const { return bRoundChambered; }

	// Whether this weapon model physically supports a chambered round (+1)
	FORCEINLINE bool					SupportsChamber()			const { return bSupportsChamberedRound; }

	// Whether a magazine is currently inserted
	FORCEINLINE bool					HasInsertedMagazine()		const { return InsertedMagazine.IsSet(); }

	// Rounds remaining in the currently inserted magazine only (0 if none)
	FORCEINLINE int32   GetMagRounds()      const { return InsertedMagazine.IsSet() ? InsertedMagazine.GetValue().CurrentRounds : 0; }

	// Total rounds available to fire: magazine + chambered round. A full 30-round mag with a round already chambered correctly shows 31/30.
	FORCEINLINE int32   GetLoadedRounds()   const { return GetMagRounds() + (bRoundChambered ? 1 : 0); }

	// Capacity of the currently inserted magazine (0 if none) — used by HUD for the /30 denominator
	FORCEINLINE int32   GetMagCapacity()    const { return InsertedMagazine.IsSet() ? InsertedMagazine.GetValue().Capacity : 0; }

	// Total rounds available (magazine + chamber) — alias kept for clarity
	FORCEINLINE int32   GetTotalRoundsAvailable() const { return GetLoadedRounds(); }

	// Damage from assigned ammo data asset; falls back to safe default
	FORCEINLINE float					GetDamage()					const { return AmmoData ? AmmoData->BaseDamage : 20.f; }

	// Full asset accessor — passed to AProjectile at spawn for per-hit damage resolution
	FORCEINLINE UAmmoData*				GetAmmoData()				const { return AmmoData; }

	// Spread / fire mode helpers — unchanged
	void AddSpreadOnFire();
	void CycleFireMode();

	FString GetCurrentFireModeDisplayName() const
	{
		switch (CurrentFireMode)
		{
		case EFireMode::EFM_Safe:		return TEXT("Safe");
		case EFireMode::EFM_SemiAuto:	return TEXT("Semi Auto");
		case EFireMode::EFM_Burst:		return TEXT("Burst");
		case EFireMode::EFM_FullAuto:	return TEXT("Full Auto");
		default:						return TEXT("Unknown");
		}
	}
	
	
	/** Config asset this weapon was initialized from. Set by InitFromConfig(). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Config")
	TObjectPtr<UWeaponConfig> WeaponConfig;

	/**
	 * Initializes the weapon from a config asset.
	 * Sets the receiver mesh, weapon AnimBP, and stores the config reference
	 * so later phases (socket queries, montage selection) can read it.
	 * Must be called immediately after spawn, before BeginPlay resolves
	 * overlap delegates.
	 */
	void InitFromConfig(UWeaponConfig* InConfig);

	/** Returns the config this weapon was initialized from, or nullptr. */
	FORCEINLINE UWeaponConfig* GetWeaponConfig() const { return WeaponConfig; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnSphereOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex
	);

private:

	// -----------------------------------------------------------------------
	// Components — unchanged
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* CollisionSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	FString PickupMessage = TEXT("Press F to pick up");

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACaseEject> CasingClass;
	
	
	// -----------------------------------------------------------------------
	// Config-assembled attachment components
	// Created at runtime by InitFromConfig — null if config field was empty.
	// -----------------------------------------------------------------------
	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Attachments")
	UStaticMeshComponent* MeshComp_Handguard = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Attachments")
	UStaticMeshComponent* MeshComp_Scope = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Attachments")
	UStaticMeshComponent* MeshComp_SightFront = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Attachments")
	UStaticMeshComponent* MeshComp_SightRear = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Weapon|Attachments")
	UStaticMeshComponent* MeshComp_Silencer = nullptr;

	// -----------------------------------------------------------------------
	// Weapon State
	// -----------------------------------------------------------------------
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	// -----------------------------------------------------------------------
	// Spread — unchanged
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float CurrentSpread;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float SpreadIncreasePerShot = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float SpreadDecayRate = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Spread")
	float MaxSpread = 1.0f;

	// -----------------------------------------------------------------------
	// Fire Mode — unchanged
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Fire Mode")
	TArray<EFireMode> AllowedFireModes = { EFireMode::EFM_SemiAuto };

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties|Fire Mode")
	EFireMode CurrentFireMode = EFireMode::EFM_SemiAuto;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Fire Mode")
	float FireRate = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Fire Mode")
	float FullAutoFireRate = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Fire Mode")
	float BurstFireRate = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Fire Mode")
	int32 BurstCount = 3;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	float WeaponRange = 1500.f;
	
	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Audio")
	bool bIsPistolClass = false;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties|Reticle")
	FReticleConfig ReticleConfig;

	// -----------------------------------------------------------------------
	// Ammo Type & Data
	// -----------------------------------------------------------------------

	// Caliber this weapon accepts — must match any inserted magazine's AmmoType
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties|Ammo")
	EAmmoType SupportedAmmoType = EAmmoType::EAT_9mm;

	// Data asset providing BaseDamage and HeadShotMultiplier for this caliber
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties|Ammo")
	UAmmoData* AmmoData = nullptr;

	// -----------------------------------------------------------------------
	// Feed Mechanism
	// -----------------------------------------------------------------------

	// How this weapon is fed — controls reload logic and chamber behavior
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties|Feed")
	EWeaponFeedType FeedType = EWeaponFeedType::EFT_DetachableMagazine;

	// Whether this weapon model physically supports a round in the chamber (+1)
	UPROPERTY(EditDefaultsOnly, Category = "Weapon Properties|Feed")
	bool bSupportsChamberedRound = true;

	// Is there currently a round in the chamber? Replicated.
	UPROPERTY(ReplicatedUsing = OnRep_LoadState)
	bool bRoundChambered = false;

	/**
	 * Replicated mirror of InsertedMagazine for non-owning clients.
	 * Gameplay logic always uses InsertedMagazine (TOptional).
	 * This is only written by SyncReplicatedLoadState() and read by OnRep_LoadState().
	 */
	UPROPERTY(ReplicatedUsing = OnRep_LoadState)
	FMagazine ReplicatedMagState;

	/**
	 * Authoritative local magazine state. Used by all gameplay logic on the
	 * server and owning client. Not directly replicated — mirrored into
	 * ReplicatedMagState via SyncReplicatedLoadState().
	 */
	TOptional<FMagazine> InsertedMagazine;

	UFUNCTION()
	void OnRep_LoadState();

	// Mirrors InsertedMagazine and bRoundChambered into replicated properties
	void SyncReplicatedLoadState();
	
	// Creates, registers, and attaches a static mesh component to WeaponMesh
	// at the given socket. Returns null if either argument is invalid.
	UStaticMeshComponent* AttachStaticMeshFromConfig(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket);

	void DecaySpread(float DeltaTime);
	
	
	// -----------------------------------------------------------------------
	// Suppressor Attachment State
	// -----------------------------------------------------------------------
	// True when a suppressor is currently equipped on this weapon.
	// Replicated so all clients know to swap audio and show/hide mesh.
	UPROPERTY(ReplicatedUsing = OnRep_SuppressorState, VisibleAnywhere, Category = "Weapon Properties|Attachments")
	bool bHasSuppressor = false;
	
	
	UFUNCTION()
	void OnRep_SuppressorState();
};