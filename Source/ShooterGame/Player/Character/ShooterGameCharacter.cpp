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
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);

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
	CameraBoom->TargetArmLength = 300.0f;								// close third-person distance
	CameraBoom->SocketOffset = FVector(0.f, 75.f, 75.f);	// slight right shoulder offset
	CameraBoom->bUsePawnControlRotation = true;							// arm follows controller pitch + yaw
	CameraBoom->bInheritPitch = true;									// allow pitch
	CameraBoom->bInheritRoll = false;									 // no roll
	CameraBoom->bInheritYaw = true;
	CameraBoom->bDoCollisionTest = true;								// camera pulls in on walls
	CameraBoom->ProbeSize = 12.f;										// collision probe radius

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	
	
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
	
}

void AShooterGameCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AShooterGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	AimOffsetTick(DeltaTime);
}

void AShooterGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Camera
		
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::Move);
		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::Look);
		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::CrouchButtonPressed);
		
		
		//EnhancedInputComponent->BindAction(PrimaryInteractAction, ETriggerEvent::Started, this, &AShooterGamePlayerController::PrimaryInteract);
		EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &AShooterGameCharacter::EquipButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::ToggleAim);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterGameCharacter::FireButtonReleased);
		EnhancedInputComponent->BindAction(CycleFireModeAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::CycleFireModeButtonPressed);
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
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AShooterGameCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AShooterGameCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);  
	}
}

void AShooterGameCharacter::AimOffsetTick(float DeltaTime)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	const bool bLockToCamera = bUseControllerRotationYaw;
	const float Speed2D = GetVelocity().Size2D();

	if (!bLockToCamera)
	{
		AimOffset_Yaw = FMath::FInterpTo(AimOffset_Yaw, 0.f, DeltaTime, 15.f);
		AimOffset_Pitch = FMath::FInterpTo(AimOffset_Pitch, 0.f, DeltaTime, 15.f);
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	const FRotator ControlRotation = GetControlRotation();
	const FRotator ActorRotation = GetActorRotation();
	const FRotator DeltaRotation = UKismetMathLibrary::NormalizedDeltaRotator(ControlRotation, ActorRotation);

	AimOffset_Yaw = FMath::FInterpTo(AimOffset_Yaw, DeltaRotation.Yaw, DeltaTime, 15.f);

	const float NormalizedPitch = FRotator::NormalizeAxis(ControlRotation.Pitch);
	AimOffset_Pitch = FMath::FInterpTo(AimOffset_Pitch, NormalizedPitch, DeltaTime, 15.f);
	AimOffset_Pitch = FMath::Clamp(AimOffset_Pitch, -90.f, 90.f);

	if (Speed2D > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	if (AimOffset_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AimOffset_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	else
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
}

void AShooterGameCharacter::SetRotationMode(bool bLockToCamera)
{
	bUseControllerRotationYaw = bLockToCamera;
	GetCharacterMovement()->bOrientRotationToMovement = !bLockToCamera;

	if (!bLockToCamera)
	{
		AimOffset_Yaw = 0.f;
	}
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




