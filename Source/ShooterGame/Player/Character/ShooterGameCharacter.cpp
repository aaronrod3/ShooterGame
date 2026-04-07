// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameCharacter.h"
#include "ShooterGame/Player/Animation/PlayerAnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "Framework/ShooterGame.h"
#include "Inventory/InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "Kismet/KismetMathLibrary.h"


DEFINE_LOG_CATEGORY(LogShooterGameCharacter);

AShooterGameCharacter::AShooterGameCharacter()
{
	
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 2000.0f;								// close third-person distance
	CameraBoom->SetRelativeRotation(FRotator(-70.f, 0.f, 0.f));	
	//CameraBoom->SocketOffset = FVector(0.f, 75.f, 75.f);	// slight right shoulder offset
	CameraBoom->bUsePawnControlRotation = false;							// arm follows controller pitch + yaw
	CameraBoom->bInheritPitch = false;									// allow pitch
	CameraBoom->bInheritRoll = false;									 // no roll
	CameraBoom->bInheritYaw = false;
	CameraBoom->bDoCollisionTest = false;								// camera pulls in on walls
	
	CurrentArmLength = 2000.f;
	TargetArmLength  = 2000.f;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	DownedComp = CreateDefaultSubobject<UDownedComponent>(TEXT("DownedComponent"));
	ReviveComp = CreateDefaultSubobject<UReviveComponent>(TEXT("ReviveComponent"));
	
	
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
	
}

void AShooterGameCharacter::BeginPlay()
{
	Super::BeginPlay();

	
	// ── initialize camera arm to top-down position for all clients ──
	if (CameraBoom)
	{
		float CurrentPitch = CameraBoom->GetRelativeRotation().Pitch;
		CameraBoom->SetRelativeRotation(FRotator(CurrentPitch, DesiredYaw, 0.f));
	}
}

void AShooterGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	FaceTowardCursor(DeltaTime);
	
	// ── Drive spring arm yaw from DesiredYaw, preserve relative pitch, never touch controller ──
	if (CameraBoom)
	{
		float CurrentPitch = CameraBoom->GetRelativeRotation().Pitch;

		DesiredYaw = FMath::FInterpTo(DesiredYaw, TargetYaw, DeltaTime, RotationSpeed);

		// Snap to exact target when close — prevents interp never arriving
		if (FMath::Abs((float)FMath::FindDeltaAngleDegrees(DesiredYaw, TargetYaw)) < 0.5f)
		{
			DesiredYaw = TargetYaw;	
		}

		CameraBoom->SetRelativeRotation(FRotator(CurrentPitch, DesiredYaw, 0.f));

		// ── Interpolate zoom (arm length) toward TargetArmLength ──
		CurrentArmLength = FMath::FInterpTo(
			CurrentArmLength,
			TargetArmLength,
			DeltaTime,
			ZoomInterpSpeed
		);
		CameraBoom->TargetArmLength = CurrentArmLength;
		// ──────────────────────────────────────────────────────────
	}
}

void AShooterGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Camera
		EnhancedInputComponent->BindAction(RotateCamera_Action, ETriggerEvent::Started, this, &AShooterGameCharacter::RotateCamera);
		EnhancedInputComponent->BindAction(ZoomCamera_Action, ETriggerEvent::Triggered, this, &AShooterGameCharacter::ZoomCamera);
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::Move);
		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::CrouchButtonPressed);
		
		
		//EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &AShooterGamePlayerController::PrimaryInteract);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &AShooterGameCharacter::EquipButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::ToggleAim);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started,   this, &AShooterGameCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterGameCharacter::FireButtonReleased);
		EnhancedInputComponent->BindAction(CycleFireModeAction, ETriggerEvent::Started, this, &AShooterGameCharacter::CycleFireModeButtonPressed);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AShooterGameCharacter::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Started,this, &AShooterGameCharacter::RevivePressed);
		EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Completed,this, &AShooterGameCharacter::ReviveReleased);
		EnhancedInputComponent->BindAction(ReviveAction, ETriggerEvent::Canceled,this, &AShooterGameCharacter::ReviveReleased);
	}
	else
	{
		UE_LOG(LogShooterGame, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AShooterGameCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	if (Combat)
	{
		Combat->Character = this;
	}
}

void AShooterGameCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(AShooterGameCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AShooterGameCharacter, Health);
}



void AShooterGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AShooterGameCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		const FRotator ArmRotation = FRotator(0.f, DesiredYaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(ArmRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection   = FRotationMatrix(ArmRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}


void AShooterGameCharacter::RotateCamera(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	// Ignore new input until current rotation has nearly landed
	if (FMath::Abs((float)FMath::FindDeltaAngleDegrees(DesiredYaw, TargetYaw)) > 45.f) return;

	const float Direction = FMath::Sign(Axis);
	TargetYaw += 90.f * Direction;
}

void AShooterGameCharacter::ZoomCamera(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	// Positive axis (scroll up) = zoom in (shorter arm length)
	// Negative axis (scroll down) = zoom out (longer arm length)
	TargetArmLength = FMath::Clamp(
		TargetArmLength - (Axis * ZoomStep),
		MinZoomDistance,
		MaxZoomDistance
	);
}


void AShooterGameCharacter::SetZoomRange(float NewMin, float NewMax)
{
	MinZoomDistance = NewMin;
	MaxZoomDistance = NewMax;

	// Immediately clamp current target into the new range
	// so the camera doesn't sit outside the new bounds until the player scrolls
	TargetArmLength = FMath::Clamp(TargetArmLength, MinZoomDistance, MaxZoomDistance);
}

void AShooterGameCharacter::FaceTowardCursor(float DeltaTime)
{
	if (!IsLocallyControlled()) return;
	
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;
	
	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursorByChannel(TraceTypeQuery1, false, Hit))
	{
		FVector LookAtCursor = (Hit.ImpactPoint - GetActorLocation()).GetSafeNormal2D();
		if (LookAtCursor.IsNearlyZero()) return;

		FRotator TargetRotation  = LookAtCursor.Rotation();
		FRotator CurrentRotation = GetActorRotation();

		// Compute aim offset BEFORE interp
		if (Combat && Combat->EquippedWeapon)
		{
			FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CurrentRotation);
			AimOffset_Yaw = Delta.Yaw;
		}
		else
		{
			AimOffset_Yaw = 0.f;
		}

		float AbsDelta = FMath::Abs(AimOffset_Yaw);
		if (AbsDelta < 5.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		else
		{
			TurnInPlace(DeltaTime);
		}

		// Interp body toward cursor — pitch always zeroed (level aiming)
		FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FaceCursorInterpSpeed);
		SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));				// ← Z=0 enforces no pitch

		if (!HasAuthority())
		{
			ServerSetFacingYaw(NewRotation.Yaw);
		}
	}
}

void AShooterGameCharacter::ServerSetFacingYaw_Implementation(float Yaw)
{
	SetActorRotation(FRotator(0, Yaw, 0));
}


void AShooterGameCharacter::TurnInPlace(float DeltaTime)
{
	if (AimOffset_Yaw > 90.f) // may narrow down later
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AimOffset_Yaw < -90.f) // may narrow down later
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
}

void AShooterGameCharacter::OnRep_DesiredYaw()
{
	// Fires on simulated proxies when the server replicates DesiredYaw.
	// Applies the spring arm yaw so remote players see the correct camera angle.
	if (CameraBoom)
	{
		const float CurrentPitch = CameraBoom->GetRelativeRotation().Pitch;
		CameraBoom->SetRelativeRotation(FRotator(CurrentPitch, DesiredYaw, 0.f));
	}
}


void AShooterGameCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AShooterGameCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}


void AShooterGameCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

/*
void AShooterGameCharacter::PrimaryInteract()
{
	UE_LOG(LogTemp, Warning, TEXT("Primary Interact"));
}
*/


void AShooterGameCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
	
}

void AShooterGameCharacter::ServerEquipButtonPressed_Implementation()
{
	if (DownedComp && !DownedComp->IsAlive()) return;
	
	if (Combat && OverlappingWeapon)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

bool AShooterGameCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

AWeapon* AShooterGameCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

bool AShooterGameCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void AShooterGameCharacter::ToggleAim()
{
	if (Combat)
	{
		Combat->SetAiming(!Combat->bAiming);
	}
}

void AShooterGameCharacter::FireButtonPressed()
{
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AShooterGameCharacter::FireButtonReleased()
{
	if (Combat) Combat->FireButtonReleased();
}

void AShooterGameCharacter::CycleFireModeButtonPressed()
{
	if (Combat) Combat->CycleFireMode();
}

void AShooterGameCharacter::ReloadButtonPressed()
{
	if (!Combat) return;
	Combat->ReloadEquippedWeapon();
}


void AShooterGameCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
	
}

void AShooterGameCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AShooterGameCharacter::MulticastHit_Implementation()
{
	PlayHitReactMontage();
}



float AShooterGameCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority()) return ActualDamage;

	// -----------------------------------------------------------------------
	// Downed — hit spikes bleedout rate, does NOT reduce health further
	// -----------------------------------------------------------------------
	if (DownedComp && DownedComp->IsDowned())
	{
		DownedComp->ServerApplyHitSpike();
		UE_LOG(LogTemp, Log,
			TEXT("AShooterGameCharacter::TakeDamage — %s hit while downed (spike applied)"),
			*GetName());
		return ActualDamage;
	}

	// -----------------------------------------------------------------------
	// Dead — ignore all damage
	// -----------------------------------------------------------------------
	if (DownedComp && DownedComp->IsDead())
	{
		return 0.f;
	}

	// -----------------------------------------------------------------------
	// Alive — apply damage to health
	// -----------------------------------------------------------------------
	Health = FMath::Clamp(Health - ActualDamage, 0.f, MaxHealth);

	UE_LOG(LogTemp, Log,
		TEXT("AShooterGameCharacter::TakeDamage — %.1f damage | Health: %.1f / %.1f"),
		ActualDamage, Health, MaxHealth);

	// Transition to downed when health hits zero
	if (Health <= 0.f && DownedComp && DownedComp->IsAlive())
	{
		DownedComp->ServerGoDown();
	}

	return ActualDamage;
}

void AShooterGameCharacter::OnRep_Health()
{
	// TODO: update HUD health bar here
	// TODO: trigger hit flash / pain sound here
	UE_LOG(LogTemp, Log, TEXT("OnRep_Health — Health: %.1f"), Health);
}



void AShooterGameCharacter::SetHealth(float NewHealth)
{
	// Authority only — clients receive via OnRep_Health
	if (!HasAuthority()) return;
	Health = FMath::Clamp(NewHealth, 0.f, MaxHealth);
}

float AShooterGameCharacter::GetBaseWalkSpeed() const
{
	// Reads from CombatComponent which owns the authoritative base speed
	if (Combat)
	{
		return Combat->GetBaseWalkSpeed();
	}
	return 600.f; // fallback
}

void AShooterGameCharacter::RevivePressed()
{
	if (ReviveComp)
	{
		ReviveComp->RevivePressed();
	}
}

void AShooterGameCharacter::ReviveReleased()
{
	if (ReviveComp)
	{
		ReviveComp->ReviveReleased();
	}
}
