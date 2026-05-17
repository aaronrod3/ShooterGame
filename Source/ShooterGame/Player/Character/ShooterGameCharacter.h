// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Inventory/EquippedStateComponent.h"
#include "Inventory/InventoryComponent.h"
#include "ShooterGame/Components/DownedComponent.h"
#include "ShooterGame/Components/ReviveComponent.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/Components/LoadoutComponent.h"
#include "ShooterGame/Types/TurningInPlace.h"
#include "Items/Weapon/Weapon.h"
#include "Logging/LogMacros.h"
#include "GenericTeamAgentInterface.h" 
#include "ShooterGameCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class AAmmoPickup;
class AVendorNPCActor;
struct FInputActionValue;
struct FVendorTransactionResult;

DECLARE_LOG_CATEGORY_EXTERN(LogShooterGameCharacter, Log, All);


UCLASS(config=Game)
class AShooterGameCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

	
public:
	AShooterGameCharacter();	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	
	// IGenericTeamAgentInterface — Team 0 = Player faction
	virtual FGenericTeamId GetGenericTeamId() const override { return FGenericTeamId(0); }
	
	// Overrides AActor::TakeDamage — called by UGameplayStatics::ApplyPointDamage
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
	void PlayFireMontage(bool bAiming);
	void PlayHitReactMontage();
	
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastHit();
	
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	FORCEINLINE ETeam GetTeam() const { return Team; }
	
	FORCEINLINE float GetAimOffset_Yaw()						const { return AimOffset_Yaw; }
	FORCEINLINE ETurningInPlace GetTurningInPlace()				const { return TurningInPlace; }
	FORCEINLINE UInventoryComponent* GetInventory()				const { return Inventory; }
	FORCEINLINE UDownedComponent* GetDownedComponent()			const { return DownedComp; }
	FORCEINLINE UReviveComponent* GetReviveComponent()			const { return ReviveComp; }
	FORCEINLINE UCombatComponent* GetCombat()					const { return Combat; }
	FORCEINLINE ULoadoutComponent* GetLoadoutComponent()		const { return LoadoutComp; }
	FORCEINLINE UEquippedStateComponent* GetEquippedState()		const { return EquippedStateComp; }
	
	FORCEINLINE float GetHealth()							const { return Health; }
	FORCEINLINE float GetMaxHealth()						const { return MaxHealth; }
	void SetHealth(float NewHealth);
	
	
	/* TPS Camera Settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	float HipFireArmLength = 325.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	float ADSArmLength = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	FVector HipFireSocketOffset = FVector(0.f, 60.f, 20.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	FVector ADSSocketOffset = FVector(0.f, 75.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	float HipFireFOV = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	float ADSFOV = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | TPS")
	float CameraInterpSpeed = 10.f;
	
	
	/* Shoulder Swap Settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Shoulder")
	float RightShoulderOffsetY = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Shoulder")
	float LeftShoulderOffsetY = -60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Shoulder")
	float ADSRightShoulderOffsetY = 75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Shoulder")
	float ADSLeftShoulderOffsetY = -75.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Shoulder")
	float ShoulderSwapInterpSpeed = 12.f;
	
	
	/* Prone Settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Prone")
	float ProneSocketOffsetZ = -60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Prone")
	float ProneWalkSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Prone")
	float ProneArmLength = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Prone")
	float ProneCapsuleHalfHeight = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Prone")
	float StandingCapsuleHalfHeight = 96.f;
	
	
	
	// -- Team settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	ETeam Team = ETeam::ET_Players; // all human players share this for now
	
	AWeapon* GetEquippedWeapon();
	
	/* Inputs */
	void Look(const FInputActionValue& Value);	
	void Move(const FInputActionValue& Value);
	void SetRotationMode(bool bLockToCamera);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void GoProneButtonPressed();
	void ApplyProneState(bool bProne);
	void SetOverlappingWeapon(AWeapon* Weapon);
	void SetOverlappingAmmoPickup(AAmmoPickup* AmmoPickup);
	void ToggleAim();
	void SetOrientationForAiming(bool bAiming); // TPS body orientation toggle
	void SwapShoulder();
	void FireButtonPressed();
	void FireButtonReleased();
	void CycleFireModeButtonPressed();
	void ReloadButtonPressed();
	void ToggleSuppressor_Input(const FInputActionValue& Value);
	void RevivePressed();
	void ReviveReleased();
	
	
	bool IsWeaponEquipped();
	bool IsAiming();
    
	UPROPERTY(ReplicatedUsing = OnRep_DesiredYaw)
	float DesiredYaw = 0.f;
	
	float TargetYaw  = 0.f;			// local only, no replication needed
	float AimOffset_Yaw;
	
	float GetBaseWalkSpeed() const;
	
	
	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float Yaw);	

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	
	
private:
	/*** COMPONENTS ***/
	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UDownedComponent* DownedComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UReviveComponent* ReviveComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UInventoryComponent* Inventory;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	ULoadoutComponent* LoadoutComp;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UEquippedStateComponent* EquippedStateComp;
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingAmmoPickup)
	class AAmmoPickup* OverlappingAmmoPickup;
	
	/** Camera  **/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	
	/* INPUT */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;
	/* Look Input Action */
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ProneAction;
	/*
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> PrimaryInteractAction;
	*/
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> EquipAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> AimAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> FireAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CycleFireModeAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ShoulderSwapAction;
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleSuppressorAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ReviveAction;
	
	UPROPERTY(EditAnywhere, Category = "Animation | Combat")
	class UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = "Animation | Combat")
	class UAnimMontage* HitReactMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ReloadMontage;
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* SuppressorMontage;
	
	// Current health — replicated so HUD stays in sync on all clients
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player Stats")
	float MaxHealth = 100.f;

	UFUNCTION()
	void OnRep_Health();
	
	
	/* Variables */
	ETurningInPlace TurningInPlace;
	
	// TPS camera interpolation state (local only, not replicated — camera is client-side)
	float CurrentArmLength = 325.f;
	float TargetArmLength  = 325.f;
	float CurrentCameraFOV = 90.f;
	bool bRightShoulder = true;
	float TargetShoulderOffsetY = 60.f;
	float LastReplicatedYaw = 0.f; // throttle ServerSetFacingYaw RPC calls
	
	/*** FUNCTIONS ***/
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	UFUNCTION(Server, Reliable)
	void ServerCollectAmmo();
	
	

	UFUNCTION()
	void OnRep_DesiredYaw();
	
	UFUNCTION()
	void OnLoadoutChanged_Internal(const FLoadoutData& NewLoadout);
	UFUNCTION()
	void OnAppearanceChanged_Internal(const FCharacterAppearance& NewAppearance);
	
	
	void TurnInPlace(float DeltaTime);
	
	UFUNCTION(Server, Reliable)
	void ServerSetProne(bool bNewProne);
	UPROPERTY(ReplicatedUsing = OnRep_IsProne)
	bool bIsProne = false;
	UFUNCTION()
	void OnRep_IsProne();
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	UFUNCTION()
	void OnRep_OverlappingAmmoPickup(AAmmoPickup* LastAmmoPickup);

	

};

