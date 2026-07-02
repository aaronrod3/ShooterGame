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
#include "ShooterGame/Components/HitZoneComponent.h"
#include "ShooterGame/Interaction/Interactable.h"
#include "ShooterGame/Interaction/Highlightable.h"
#include "ShooterGame/Types/TurningInPlace.h"
#include "ShooterGame/HUD/InteractPromptWidget.h"
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
	void PlayReloadMontage();
	void PlayInteractionMontage();
	// M6 — config-driven montage play functions
	// Each reads the montage from the equipped weapon's WeaponConfig.
	// No hardcoded asset reference. Call sites are in CombatComponent and
	// on input actions (mag check / inspect input bindings deferred).
	void PlayEquipMontage();
	void PlayFireModeMontage();
	void PlayMagCheckMontage();
	void PlayInspectMontage();
	
	bool IsReloadAnimationPlaying() const;
	bool IsInteractionAnimationPlaying() const;
	
	void StartInteractionAnimation();
	void StopInteractionAnimation();

	// -----------------------------------------------------------------------
	// Notify-driven stubs (infima_integration_plan.md Section 3) — bodies not
	// yet implemented, log a warning until a design pass wires them up.
	// -----------------------------------------------------------------------

	/** Called by AN_SpawnObjectAttached to spawn and attach a prop to a socket. */
	UFUNCTION(BlueprintCallable, Category = "Anim|Notify")
	virtual void SpawnObjectAttached(TSubclassOf<AActor> ObjectToSpawn, FName SocketName, FVector LocationOffset, FRotator RotationOffset, float VisibilityDelay);

	/** Called by AN_ThrowPhysicsObject to detach and throw a socket-attached prop. */
	UFUNCTION(BlueprintCallable, Category = "Anim|Notify")
	virtual void ThrowPhysicsObject(TSubclassOf<AActor> ObjectToSpawn, FName SocketName, FVector LocationOffset, FRotator RotationOffset, float ThrowForce, float ThrowRotationForce, bool bDestroySocketItem);

	UFUNCTION(Client, Reliable)
	void ClientPlayInteractionMontage();
	
	void SetInteractionAnimationRequested(bool bRequested);
	
	
	FName GetLeftHandIKSocketName() const { return LeftHandIKSocketName; }
	FName GetRightHandIKBoneName() const { return RightHandIKBoneName; }
	FVector GetLeftHandIKLocationOffset() const { return LeftHandIKLocationOffset; }
	FRotator GetLeftHandIKRotationOffset() const { return LeftHandIKRotationOffset; }
	
	
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
	
	FORCEINLINE float GetAimOffset_Yaw()							const { return AimOffset_Yaw; }
	FORCEINLINE float GetAimOffset_Pitch()						const { return AimOffset_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace()				const { return TurningInPlace; }
	FORCEINLINE UInventoryComponent* GetInventory()				const { return Inventory; }
	FORCEINLINE UDownedComponent* GetDownedComponent()			const { return DownedComp; }
	FORCEINLINE UReviveComponent* GetReviveComponent()			const { return ReviveComp; }
	FORCEINLINE UCombatComponent* GetCombat()					const { return Combat; }
	FORCEINLINE ULoadoutComponent* GetLoadoutComponent()		const { return LoadoutComp; }
	FORCEINLINE UEquippedStateComponent* GetEquippedState()		const { return EquippedStateComp; }
	FORCEINLINE UHitZoneComponent* GetHitZoneComponent()		const { return HitZoneComponent; }
	FORCEINLINE bool IsInteractionAnimationRequested()			const { return bInteractionAnimationRequested; }
	FORCEINLINE UAnimMontage* GetMontage_Fire()					const { return Montage_Fire; }
	FORCEINLINE UAnimMontage* GetMontage_Fire_Empty()			const { return Montage_Fire_Empty; }
	FORCEINLINE UAnimMontage* GetMontage_Reload()				const { return Montage_Reload; }
	FORCEINLINE UAnimMontage* GetMontage_Reload_Empty()			const { return Montage_Reload_Empty; }
	FORCEINLINE UAnimMontage* GetMontage_Reload_Quick()			const { return Montage_Reload_Quick; }
	FORCEINLINE UAnimMontage* GetMontage_FireModeSwitch()		const { return Montage_FireModeSwitch; }
	FORCEINLINE UAnimMontage* GetMontage_MagCheck()				const { return Montage_MagCheck; }
	
	FORCEINLINE bool GetIsProne()								const { return bIsProne; }
	FORCEINLINE bool IsSprinting()								const { return bIsSprinting; }
	FORCEINLINE bool CanSprint()								const { return bCanSprint; }
	FORCEINLINE float GetCurrentStamina()						const { return CurrentStamina; }
	FORCEINLINE float GetMaxStamina()							const { return MaxStamina; }
	FORCEINLINE FVector2D GetAnimationInputMoveVector()			const { return AnimationInputMoveVector; }
	FORCEINLINE const FTransform& GetAimDownSightsTransform()	const { return AimDownSightsTransform; }
	FORCEINLINE const FTransform& GetRecoilTransform()			const { return RecoilTransform; }
	FORCEINLINE const FTransform& GetCrouchTransform()			const { return CrouchTransform; }
	
	FORCEINLINE float GetHealth()								const { return Health; }
	FORCEINLINE float GetMaxHealth()							const { return MaxHealth; }
	void SetHealth(float NewHealth);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitZone")
	TObjectPtr<UHitZoneComponent> HitZoneComponent;
	
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
	
	// Default loadout applied on spawn before any save data is pushed.
	// Set this in BP_PlayerCharacter defaults to give the character a starting weapon and mags.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout")
	FLoadoutData DefaultLoadout;
	
	
	
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
	void StartSprinting();
	void StopSprinting();
	void FireButtonPressed();
	void FireButtonReleased();
	void CycleFireModeButtonPressed();
	void ReloadButtonPressed();
	void HighReadyButtonPressed();
	void ToggleSuppressor_Input(const FInputActionValue& Value);
	void RevivePressed();
	void ReviveReleased();
	void PrimaryInteractButtonPressed();
	
	
	bool IsWeaponEquipped();
	bool IsAiming();
    
	UPROPERTY(ReplicatedUsing = OnRep_DesiredYaw)
	float DesiredYaw = 0.f;
	
	float TargetYaw  = 0.f;			// local only, no replication needed
	float AimOffset_Yaw = 0.f;
	float AimOffset_Pitch = 0.f;
	
	float GetBaseWalkSpeed() const;
	
	// Interact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractTraceDistance = 350.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractTraceRadius = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bDrawInteractTraceDebug = false;
	
	
	UFUNCTION(Server, Unreliable)
	void ServerSetFacingYaw(float Yaw);	

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	
	UAnimMontage* GetInteractionMontageForCurrentStance() const;
	
	
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
	
	// --- Interaction Prompt Widget ---

	// Assign WBP_InteractPrompt in the character Blueprint
	UPROPERTY(EditDefaultsOnly, Category = "Interaction | UI")
	TSubclassOf<UInteractPromptWidget> InteractPromptWidgetClass;

	// Runtime instance — created in BeginPlay, never replicated
	UPROPERTY()
	TObjectPtr<UInteractPromptWidget> InteractPromptWidgetInstance;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Interaction", meta = (AllowPrivateAccess = "true"))
	bool bInteractionAnimationRequested = false;
	
	UFUNCTION()
	void AnimNotify_InteractionFinished();
	
	
	
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
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> PrimaryInteractAction;
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
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* HighReadyAction;
	
	/* MONTAGES */
	// -----------------------------------------------------------------------
	// assign in BP defaults

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Fire")
	UAnimMontage* Montage_Fire = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Fire")
	UAnimMontage* Montage_Fire_Empty = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Reload")
	UAnimMontage* Montage_Reload = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Reload")
	UAnimMontage* Montage_Reload_Empty = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Reload")
	UAnimMontage* Montage_Reload_Quick = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_FireModeSwitch = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_MagCheck = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_ClearJam_MagSwipe = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_ClearJam_Rack = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_Grenade_Throw = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_Melee_Bash = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_Melee_Swing_L = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages|Actions")
	UAnimMontage* Montage_Melee_Swing_R = nullptr;

	// Keep these for non-Infima paths
	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	UAnimMontage* HitReactMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	UAnimMontage* SuppressorMontage = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	UAnimMontage* InteractionMontage_Unarmed = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	UAnimMontage* InteractionMontage_Pistol = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Animation|Montages")
	UAnimMontage* InteractionMontage_Rifle = nullptr;
	
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
	
	// -----------------------------------------------------------------------
	// Phase 1 animation bridge / sprint contract
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true"))
	float RunSpeed = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	float MaxStamina = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	float CurrentStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintDrainRate = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintRecoveryRate = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	float SprintMinStaminaToStart = 10.f;
	
	/**
	 * When false (default): hold shift to sprint, release to stop.
	 * When true: first press starts sprint, second press stops it.
	 * Exposed so a settings menu can toggle this at runtime via Blueprint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bSprintToggleMode = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bIsSprinting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Sprint", meta = (AllowPrivateAccess = "true"))
	bool bCanSprint = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Movement", meta = (AllowPrivateAccess = "true"))
	FVector2D AnimationInputMoveVector = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Procedural", meta = (AllowPrivateAccess = "true"))
	FTransform AimDownSightsTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Procedural", meta = (AllowPrivateAccess = "true"))
	FTransform RecoilTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Procedural", meta = (AllowPrivateAccess = "true"))
	FTransform CrouchTransform = FTransform::Identity;
	
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
	
	UPROPERTY()
	AActor* FocusedInteractableActor = nullptr;

	UPROPERTY()
	AActor* CurrentPromptActor = nullptr;

	UPROPERTY()
	AActor* CurrentHighlightedActor = nullptr;
	
	
	void UpdateInteractFocus();
	AActor* FindBestInteractableInView(FHitResult& OutHit) const;
	void SetCurrentPromptActor(AActor* NewPromptActor);
	void RefreshCurrentPromptWidget();
	void ClearCurrentPromptWidget();
	AActor* ResolveBestInteractionCandidate() const;
	
	UFUNCTION(Server, Reliable)
	void ServerPrimaryInteract();

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|IK", meta = (AllowPrivateAccess = "true"))
	FName LeftHandIKSocketName = FName("LeftHandSocket");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|IK", meta = (AllowPrivateAccess = "true"))
	FName RightHandIKBoneName = FName("ik_hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|IK", meta = (AllowPrivateAccess = "true"))
	FVector LeftHandIKLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|IK", meta = (AllowPrivateAccess = "true"))
	FRotator LeftHandIKRotationOffset = FRotator::ZeroRotator;
	

};

