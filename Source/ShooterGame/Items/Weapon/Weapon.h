// Source/ShooterGame/Items/Weapon/Weapon.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Types/FireMode.h"
#include "ShooterGame/Types/AmmoType.h"
#include "ShooterGame/Items/Ammo/WeaponFeedTypes.h"
#include "ShooterGame/Items/Ammo/AmmoData.h"
#include "Weapon.generated.h"


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

	void ShowPickupWidget(bool bShowWidget);
	virtual void Fire(const FVector& HitTarget);
	void SetWeaponState(EWeaponState State);
	void ApplySpreadMultiplier(float Multiplier);

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
	// Accessors
	// -----------------------------------------------------------------------

	FORCEINLINE USphereComponent*		GetAreaSphere()				const { return CollisionSphere; }
	FORCEINLINE USkeletalMeshComponent*	GetWeaponMesh()				const { return WeaponMesh; }
	FORCEINLINE float					GetCurrentSpread()			const { return CurrentSpread; }
	FORCEINLINE float					GetMaxSpread()				const { return MaxSpread; }
	FORCEINLINE float					GetWeaponRange()			const { return WeaponRange; }
	FORCEINLINE const FReticleConfig&	GetReticleConfig()			const { return ReticleConfig; }
	FORCEINLINE EFireMode				GetCurrentFireMode()		const { return CurrentFireMode; }
	FORCEINLINE float					GetFireRate()				const { return FireRate; }
	FORCEINLINE float					GetFullAutoFireRate()		const { return FullAutoFireRate; }
	FORCEINLINE float					GetBurstFireRate()			const { return BurstFireRate; }
	FORCEINLINE int32					GetBurstCount()				const { return BurstCount; }
	FORCEINLINE bool					IsFireModeAllowed(EFireMode Mode) const { return AllowedFireModes.Contains(Mode); }

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

	// Rounds remaining in the currently inserted magazine (0 if none)
	FORCEINLINE int32					GetLoadedRounds()			const { return InsertedMagazine.IsSet() ? InsertedMagazine.GetValue().CurrentRounds : 0; }

	// Capacity of the currently inserted magazine (0 if none)
	FORCEINLINE int32					GetMagCapacity()			const { return InsertedMagazine.IsSet() ? InsertedMagazine.GetValue().Capacity : 0; }

	// Total rounds available (magazine + chamber)
	FORCEINLINE int32					GetTotalRoundsAvailable()	const { return GetLoadedRounds() + (bRoundChambered ? 1 : 0); }

	// Damage from assigned ammo data asset; falls back to safe default
	FORCEINLINE float					GetDamage()					const { return AmmoData ? AmmoData->BaseDamage : 20.f; }
	FORCEINLINE float					GetHeadShotMultiplier()		const { return AmmoData ? AmmoData->HeadShotMultiplier : 2.f; }

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
	class UAnimationAsset* FireAnimation;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACaseEject> CasingClass;

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

	void DecaySpread(float DeltaTime);
};