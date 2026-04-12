// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Inventory/InventoryComponent.h"
#include "ShooterGame/Components/DownedComponent.h"
#include "ShooterGame/Components/ReviveComponent.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/Types/TurningInPlace.h"
#include "Items/Weapon/Weapon.h"
#include "Logging/LogMacros.h"
#include "ShooterGameCharacter.generated.h"


class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class AAmmoPickup;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogShooterGameCharacter, Log, All);


UCLASS()
class SHOOTERGAME_API AShooterGameCharacter : public ACharacter
{
	GENERATED_BODY()

	
public:
	AShooterGameCharacter();	
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	
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
	
	FORCEINLINE float GetAimOffset_Yaw()				const { return AimOffset_Yaw; }
	FORCEINLINE ETurningInPlace GetTurningInPlace()		const { return TurningInPlace; }
	FORCEINLINE UInventoryComponent* GetInventory()		const { return Inventory; }
	FORCEINLINE UDownedComponent* GetDownedComponent()	const { return DownedComp; }
	FORCEINLINE UReviveComponent* GetReviveComponent()  const { return ReviveComp; }
	FORCEINLINE UCombatComponent* GetCombat()			const { return Combat; }
	
	FORCEINLINE float GetHealth()						const { return Health; }
	FORCEINLINE float GetMaxHealth()					const { return MaxHealth; }
	void SetHealth(float NewHealth);
	
	
	/* Camera Settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float RotationSpeed = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float FaceCursorInterpSpeed = 15.f;	

	// ── Zoom Settings ──
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float MinZoomDistance = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float MaxZoomDistance = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float ZoomStep = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float ZoomInterpSpeed = 8.f;

	/** Call from Blueprint to constrain zoom range at runtime (e.g. indoors vs open world) */
	UFUNCTION(BlueprintCallable, Category = "Camera | Zoom")
	void SetZoomRange(float NewMin, float NewMax);
	// ───────────────────
	
	
	// -- Team settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	ETeam Team = ETeam::ET_Players; // all human players share this for now
	
	AWeapon* GetEquippedWeapon();
	
	/* Inputs */
	void RotateCamera(const FInputActionValue& Value);
	void ZoomCamera(const FInputActionValue& Value); 
	void FaceTowardCursor(float DeltaTime);		
	void Move(const FInputActionValue& Value);
	void SetRotationMode(bool bLockToCamera);
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void SetOverlappingWeapon(AWeapon* Weapon);
	void SetOverlappingAmmoPickup(AAmmoPickup* AmmoPickup);
	void ToggleAim();
	void FireButtonPressed();
	void FireButtonReleased();
	void CycleFireModeButtonPressed();
	void ReloadButtonPressed();
	void RevivePressed();
	void ReviveReleased();
	
	
	bool IsWeaponEquipped();
	bool IsAiming();
    
	UPROPERTY(ReplicatedUsing = OnRep_DesiredYaw)
	float DesiredYaw = 0.f;
	
	float TargetYaw  = 0.f;			// local only, no replication needed
	float AimOffset_Yaw;
	
	float GetBaseWalkSpeed() const;
	
	
	

protected:
	virtual void BeginPlay() override;
	
	
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
	TObjectPtr<UInputAction> RotateCamera_Action;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> ZoomCamera_Action;
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> CrouchAction;
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
	UInputAction* ReviveAction;
	
	UPROPERTY(EditAnywhere, Category = "Animation | Combat")
	class UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = "Animation | Combat")
	class UAnimMontage* HitReactMontage;
	
	
	// Current health — replicated so HUD stays in sync on all clients
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Player Stats")
	float MaxHealth = 100.f;

	UFUNCTION()
	void OnRep_Health();
	
	
	/* Variables */
	ETurningInPlace TurningInPlace;
	
	// Zoom interpolation state (local only, not replicated — camera is client-side)
	float CurrentArmLength = 2000.f;
	float TargetArmLength  = 2000.f;
	
	/*** FUNCTIONS ***/
	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float Yaw);	
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();
	UFUNCTION(Server, Reliable)
	void ServerCollectAmmo();
	UFUNCTION()
	void OnRep_DesiredYaw();
	
	
	void TurnInPlace(float DeltaTime);
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);
	UFUNCTION()
	void OnRep_OverlappingAmmoPickup(AAmmoPickup* LastAmmoPickup);

	

};

