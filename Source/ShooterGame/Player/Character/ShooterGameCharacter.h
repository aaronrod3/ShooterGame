// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterGame/Types/TurningInPlace.h"
#include "Items/Weapon/Weapon.h"
#include "Logging/LogMacros.h"
#include "ShooterGameCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
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
	
	FORCEINLINE float GetAimOffset_Yaw() const { return AimOffset_Yaw; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	
	
	
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
	void ToggleAim();
	void FireButtonPressed();
	void FireButtonReleased();
	void CycleFireModeButtonPressed();
	
	bool IsWeaponEquipped();
	bool IsAiming();
    
	UPROPERTY(ReplicatedUsing = OnRep_DesiredYaw)
	float DesiredYaw = 0.f;
	
	float TargetYaw  = 0.f;			// local only, no replication needed
	float AimOffset_Yaw;
	

protected:
	virtual void BeginPlay() override;
	
	
private:
	/*** COMPONENTS ***/
	UPROPERTY(VisibleAnywhere)
	class UCombatComponent* Combat;
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;
	
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
	
	UPROPERTY(EditAnywhere, Category = "Animation | Combat")
	class UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = "Animation | Combat")
	class UAnimMontage* HitReactMontage;
	
	
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
	UFUNCTION()
	void OnRep_DesiredYaw();
	
	
	void TurnInPlace(float DeltaTime);
	
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	

};

