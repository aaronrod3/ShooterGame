// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Framework/ShooterGame.h"
#include "Kismet/GameplayStatics.h"


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
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = false; // lock pitch to spring arm
	CameraBoom->bInheritRoll = false;
	CameraBoom->bInheritYaw = true; // allow yaw from actor rotation
	CameraBoom->bDoCollisionTest = false; // prevent wall clipping

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}



void AShooterGameCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	FaceTowardCursor(DeltaTime);
}

void AShooterGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Camera
		EnhancedInputComponent->BindAction(RotateCamera_Action,ETriggerEvent::Triggered, this, &AShooterGameCharacter::RotateCamera);
		
		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::Move);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::Look);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AShooterGameCharacter::Look);
	}
	else
	{
		UE_LOG(LogShooterGame, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


// Camera rotation
void AShooterGameCharacter::RotateCamera(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	DesiredYaw += Axis * RotationSpeed * GetWorld()->GetDeltaSeconds();
}


void AShooterGameCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AShooterGameCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the input
	DoLook(LookAxisVector.X, LookAxisVector.Y);
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

void AShooterGameCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AShooterGameCharacter::FaceTowardCursor(float DeltaTime)
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController) return;
	
	FHitResult Hit;
	if (PlayerController->GetHitResultUnderCursorByChannel(TraceTypeQuery1, false, Hit))
	{
		FVector LookAtCursor = (Hit.ImpactPoint - GetActorLocation()).GetSafeNormal2D();
		if (LookAtCursor.IsNearlyZero()) return;
		
		FRotator TargetRotation  = LookAtCursor.Rotation();
		FRotator CurrentRotation = GetActorRotation();
		FRotator NewRotation     = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, FaceCursorInterpSpeed);
		
		SetActorRotation(FRotator(0, NewRotation.Yaw, 0));
		
	}
}





/*** MULTIPLAYER ***/









