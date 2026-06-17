// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGameCharacter.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "ShooterGame/Player/Animation/PlayerAnimInstance.h"
#include "ShooterGame/Types/PlayerWeaponStance.h"
#include "ShooterGame/Types/CombatTypes.h"
#include "ShooterGame/Items/Ammo/AmmoPickup.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "ShooterGame/Framework/PlayerState/ShooterPlayerState.h"
#include "ShooterGame/Types/VendorTypes.h"
#include "ShooterGame/Inventory/ItemDefinition.h"
#include "ShooterGame/Framework/Subsystems/QuestTrackerSubsystem.h"
#include "ShooterGame/Interaction/Interactable.h"
#include "ShooterGame/Interaction/Highlightable.h"
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
#include "Framework/Subsystems/ShooterSaveGameSubsystem.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquippedStateComponent.h"
#include "Engine/GameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"



DEFINE_LOG_CATEGORY(LogShooterGameCharacter);

// -----------------------------------------------------------------------
// Config lookup helper
// -----------------------------------------------------------------------

/** Returns the WeaponConfig for the currently equipped weapon, or nullptr. */
static const UWeaponConfig* GetConfigForEquippedWeapon(AShooterGameCharacter* Character)
{
	if (!Character) return nullptr;
	const AWeapon* Weapon = Character->GetEquippedWeapon();
	if (!Weapon) return nullptr;
	return Weapon->GetWeaponConfig();
}

AShooterGameCharacter::AShooterGameCharacter()
{
	
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Character body does not rotate with the controller — the spring arm handles camera rotation.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// TPS orientation — body follows movement direction by default
	// bUseControllerRotationYaw is toggled dynamically when aiming
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 150.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = HipFireArmLength;
	CameraBoom->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	CameraBoom->SocketOffset = HipFireSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bDoCollisionTest = true;

	CurrentArmLength = HipFireArmLength;
	TargetArmLength  = HipFireArmLength;
	CurrentCameraFOV = HipFireFOV;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->FieldOfView = HipFireFOV;

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);
	
	Inventory			= CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	LoadoutComp			= CreateDefaultSubobject<ULoadoutComponent>(TEXT("LoadoutComponent"));
	EquippedStateComp	= CreateDefaultSubobject<UEquippedStateComponent>(TEXT("EquippedStateComp"));
	DownedComp			= CreateDefaultSubobject<UDownedComponent>(TEXT("DownedComponent"));
	ReviveComp			= CreateDefaultSubobject<UReviveComponent>(TEXT("ReviveComponent"));
	HitZoneComponent	= CreateDefaultSubobject<UHitZoneComponent>(TEXT("HitZoneComponent"));
	
	
	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	
	
	// Allows AI hearing sense to detect noise events reported with this pawn as instigator
	UAIPerceptionStimuliSourceComponent* StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->bAutoRegister = true;
	StimuliSource->RegisterForSense(TSubclassOf<UAISense>(UAISense_Hearing::StaticClass()));
	
}

void AShooterGameCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	// Temporary debug
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreLookInput(false);
		PC->SetIgnoreMoveInput(false);
		UE_LOG(LogShooterGameCharacter, Warning, TEXT("Controller found: %s"), *PC->GetName());
	}
	else
	{
		UE_LOG(LogShooterGameCharacter, Warning, TEXT("NO PLAYER CONTROLLER on BeginPlay"));
	}
	
	UAIPerceptionStimuliSourceComponent* StimuliComp =
	FindComponentByClass<UAIPerceptionStimuliSourceComponent>();

	if (!StimuliComp)
	{
		StimuliComp = NewObject<UAIPerceptionStimuliSourceComponent>(
			this, TEXT("StimuliSource"));
		StimuliComp->RegisterComponent();
	}

	StimuliComp->RegisterForSense(UAISense_Hearing::StaticClass());
	StimuliComp->RegisterWithPerceptionSystem();

	UE_LOG(LogTemp, Warning, TEXT("[STIMULI] %s registered for UAISense_Hearing"),
		*GetName());
	
	// TPS — CameraBoom uses bUsePawnControlRotation.
	// No manual rotation setup needed at BeginPlay.
	
	// Set initial TPS orientation — not aiming by default
	SetOrientationForAiming(false);
	
	// Create the interaction prompt widget — local client only
	// HasAuthority check is not enough here because listen server players
	// are both authority AND locally controlled. IsLocallyControlled is correct.
	if (IsLocallyControlled() && InteractPromptWidgetClass)
	{
		InteractPromptWidgetInstance = CreateWidget<UInteractPromptWidget>(
			GetWorld(),
			InteractPromptWidgetClass
		);

		if (InteractPromptWidgetInstance)
		{
			InteractPromptWidgetInstance->AddToViewport();
			InteractPromptWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	
	
}

void AShooterGameCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!CameraBoom || !FollowCamera) return;

    const bool bShouldADS = (Combat && Combat->bAiming);

    // Keep target shoulder offset Y in sync with aim state changes
    if (bShouldADS)
    {
        TargetShoulderOffsetY = bRightShoulder ? ADSRightShoulderOffsetY : ADSLeftShoulderOffsetY;
    }
    else
    {
        TargetShoulderOffsetY = bRightShoulder ? RightShoulderOffsetY : LeftShoulderOffsetY;
    }

    // Choose arm length from aim/prone state
    float DesiredArmLength;
    if (bIsProne)
    {
        DesiredArmLength = ProneArmLength;
    }
    else
    {
        DesiredArmLength = bShouldADS ? ADSArmLength : HipFireArmLength;
    }

    // Choose base socket offset X and Z from aim/prone state
    FVector BaseSocketOffset = bShouldADS ? ADSSocketOffset : HipFireSocketOffset;
    if (bIsProne)
    {
        BaseSocketOffset.Z = ProneSocketOffsetZ;
    }

    // Smooth arm-length transition
    CurrentArmLength = FMath::FInterpTo(
        CurrentArmLength,
        DesiredArmLength,
        DeltaTime,
        CameraInterpSpeed
    );
    CameraBoom->TargetArmLength = CurrentArmLength;

    // Smooth shoulder Y offset
    const float NewSocketOffsetY = FMath::FInterpTo(
        CameraBoom->SocketOffset.Y,
        TargetShoulderOffsetY,
        DeltaTime,
        ShoulderSwapInterpSpeed
    );

    // Smooth socket Z offset
    const float NewSocketOffsetZ = FMath::FInterpTo(
        CameraBoom->SocketOffset.Z,
        BaseSocketOffset.Z,
        DeltaTime,
        CameraInterpSpeed
    );

    // Apply final socket offset
    CameraBoom->SocketOffset = FVector(
        BaseSocketOffset.X,
        NewSocketOffsetY,
        NewSocketOffsetZ
    );

    // Smooth FOV transition
    const float DesiredFOV = bShouldADS ? ADSFOV : HipFireFOV;
    CurrentCameraFOV = FMath::FInterpTo(
        CurrentCameraFOV,
        DesiredFOV,
        DeltaTime,
        CameraInterpSpeed
    );
    FollowCamera->SetFieldOfView(CurrentCameraFOV);

    // Replicate actor yaw to server so simulated proxies see correct facing direction.
    // Only runs on the locally controlled client — not on server or proxies.
    if (IsLocallyControlled() && !HasAuthority())
    {
        const float CurrentYaw = GetActorRotation().Yaw;
        if (!FMath::IsNearlyEqual(CurrentYaw, LastReplicatedYaw, 0.5f))
        {
            ServerSetFacingYaw(CurrentYaw);
            LastReplicatedYaw = CurrentYaw;
        }
    }

    // Interaction focus — local client only, purely cosmetic
    if (IsLocallyControlled())
    {
        UpdateInteractFocus();
    }

    // -----------------------------------------------------------------------
    // Stamina drain / recovery (authority or locally controlled)
    // -----------------------------------------------------------------------
    if (IsLocallyControlled() || HasAuthority())
    {
        if (bIsSprinting)
        {
            CurrentStamina = FMath::Clamp(
                CurrentStamina - (SprintDrainRate * DeltaTime),
                0.f,
                MaxStamina
            );

            // Auto-stop sprint when stamina runs out — bypasses toggle mode
            if (CurrentStamina <= 0.f)
            {
                bIsSprinting = false;
                bCanSprint = false;
            }
        }
        else
        {
            CurrentStamina = FMath::Clamp(
                CurrentStamina + (SprintRecoveryRate * DeltaTime),
                0.f,
                MaxStamina
            );

            if (!bCanSprint && CurrentStamina >= SprintMinStaminaToStart)
            {
                bCanSprint = true;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Authoritative speed selection — single source of truth for MaxWalkSpeed
    // Precedence: prone (ApplyProneState) > sprint > run
    // ADS multiplier applied on top of current tier speed.
    // -----------------------------------------------------------------------
    if (!bIsProne)
    {
        const float TierSpeed = bIsSprinting ? SprintSpeed : RunSpeed;
        const float AimMult   = (Combat && Combat->bAiming)
            ? Combat->GetAimSpeedMultiplier()
            : 1.f;
        GetCharacterMovement()->MaxWalkSpeed = TierSpeed * AimMult;
    }

    // -----------------------------------------------------------------------
    // AnimationInputMoveVector — local-space 2D movement input for blend spaces
    // -----------------------------------------------------------------------
    {
        const FVector WorldVelocity = GetVelocity();
        const FVector ActorForward  = GetActorForwardVector();
        const FVector ActorRight    = GetActorRightVector();

        const float ForwardComponent = FVector::DotProduct(WorldVelocity, ActorForward);
        const float RightComponent   = FVector::DotProduct(WorldVelocity, ActorRight);

        AnimationInputMoveVector = FVector2D(ForwardComponent, RightComponent);
    }

    // -----------------------------------------------------------------------
    // AimOffset_Pitch — normalized control pitch for AnimBP aim offset
    // -----------------------------------------------------------------------
    if (Controller)
    {
        float RawPitch = GetControlRotation().Pitch;
        if (RawPitch > 180.f)
        {
            RawPitch -= 360.f;
        }
        AimOffset_Pitch = FMath::Clamp(RawPitch, -90.f, 90.f);
    }
}

void AShooterGameCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

		// Camera
		EnhancedInputComponent->BindAction(LookAction,				ETriggerEvent::Triggered,	this,	&AShooterGameCharacter::Look);
		// Moving
		EnhancedInputComponent->BindAction(MoveAction,				ETriggerEvent::Triggered,	this,	&AShooterGameCharacter::Move);
		// Sprint — Started/Completed pair for hold-to-sprint
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started,   this, &AShooterGameCharacter::StartSprinting);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AShooterGameCharacter::StopSprinting);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled,  this, &AShooterGameCharacter::StopSprinting);
		}
		// Crouch
		EnhancedInputComponent->BindAction(CrouchAction,				ETriggerEvent::Triggered,	this,	&AShooterGameCharacter::CrouchButtonPressed);
		if (ProneAction)
		{
			EnhancedInputComponent->BindAction(ProneAction,				ETriggerEvent::Started,		this,	&AShooterGameCharacter::GoProneButtonPressed);
		}
		
		
		if (PrimaryInteractAction)
		{
			EnhancedInputComponent->BindAction(
				PrimaryInteractAction,
				ETriggerEvent::Started,
				this,
				&AShooterGameCharacter::PrimaryInteractButtonPressed
			);
		}
		
		EnhancedInputComponent->BindAction(EquipAction,				ETriggerEvent::Started,		this,	&AShooterGameCharacter::EquipButtonPressed);
		EnhancedInputComponent->BindAction(AimAction,				ETriggerEvent::Triggered,	this,	&AShooterGameCharacter::ToggleAim);
		EnhancedInputComponent->BindAction(ShoulderSwapAction,			ETriggerEvent::Started,		this,	&AShooterGameCharacter::SwapShoulder);
		EnhancedInputComponent->BindAction(FireAction,				ETriggerEvent::Started,		this,	&AShooterGameCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction,				ETriggerEvent::Completed,	this,	&AShooterGameCharacter::FireButtonReleased);
		EnhancedInputComponent->BindAction(CycleFireModeAction,		ETriggerEvent::Started,		this,	&AShooterGameCharacter::CycleFireModeButtonPressed);
		EnhancedInputComponent->BindAction(ReloadAction,				ETriggerEvent::Started,		this,	&AShooterGameCharacter::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(ToggleSuppressorAction,	ETriggerEvent::Started,		this,	&AShooterGameCharacter::ToggleSuppressor_Input);
		EnhancedInputComponent->BindAction(ReviveAction,				ETriggerEvent::Started,		this,	&AShooterGameCharacter::RevivePressed);
		EnhancedInputComponent->BindAction(ReviveAction,				ETriggerEvent::Completed,	this,	&AShooterGameCharacter::ReviveReleased);
		EnhancedInputComponent->BindAction(ReviveAction,				ETriggerEvent::Canceled,	this,	&AShooterGameCharacter::ReviveReleased);
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
		Combat->Character = this;

	if (LoadoutComp)
	{
		LoadoutComp->OnLoadoutChanged.AddDynamic(this, &AShooterGameCharacter::OnLoadoutChanged_Internal); // inventory first
		LoadoutComp->OnLoadoutChanged.AddDynamic(Combat, &UCombatComponent::OnLoadoutUpdated);             // weapon second
		LoadoutComp->OnAppearanceChanged.AddDynamic(this, &AShooterGameCharacter::OnAppearanceChanged_Internal);
	}
}

void AShooterGameCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    
	DOREPLIFETIME_CONDITION(AShooterGameCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AShooterGameCharacter, OverlappingAmmoPickup, COND_OwnerOnly);
	DOREPLIFETIME(AShooterGameCharacter, Health);
	DOREPLIFETIME(AShooterGameCharacter, bIsProne);
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
		// Use control rotation yaw so movement is always relative to camera facing
		const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AShooterGameCharacter::StartSprinting()
{
	if (Combat) { Combat->ExitCombatState(); }
	
	if (DownedComp && !DownedComp->IsAlive()) return;
	if (Combat && Combat->bAiming) return;

	if (bSprintToggleMode)
	{
		// Toggle: first press starts, second press stops
		if (bIsSprinting)
		{
			StopSprinting();
			return;
		}
	}

	if (!bCanSprint) return;

	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AShooterGameCharacter::StopSprinting()
{
	if (!bIsSprinting) return;

	// In toggle mode the Completed event fires when the key is released,
	// but we only want to stop on the next press — so ignore the release.
	if (bSprintToggleMode) return;

	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = RunSpeed;
}


void AShooterGameCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (GetController() != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void AShooterGameCharacter::ServerSetFacingYaw_Implementation(float Yaw)
{
	// Only apply manual facing yaw when not in ADS orientation mode.
	// During ADS, bUseControllerRotationYaw handles body rotation on the server.
	if (Combat && !Combat->bAiming)
	{
		SetActorRotation(FRotator(0.f, Yaw, 0.f));
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

void AShooterGameCharacter::OnRep_DesiredYaw()
{
	// DesiredYaw was used in top-down camera system.
	// TPS uses bUsePawnControlRotation on CameraBoom — no action needed here.
	// Keeping the replicated property for now to avoid breaking existing saves/packages.
}


AActor* AShooterGameCharacter::FindBestInteractableInView(FHitResult& OutHit) const
{
	if (!FollowCamera) return nullptr;

	const FVector TraceStart = FollowCamera->GetComponentLocation();
	const FVector TraceEnd   = TraceStart + (FollowCamera->GetForwardVector() * InteractTraceDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	TArray<UPrimitiveComponent*> OwnedPrimitives;
	GetComponents<UPrimitiveComponent>(OwnedPrimitives);
	for (UPrimitiveComponent* Comp : OwnedPrimitives)
	{
		QueryParams.AddIgnoredComponent(Comp);
	}
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	for (AActor* Attached : AttachedActors)
	{
		QueryParams.AddIgnoredActor(Attached);
	}
	QueryParams.bReturnPhysicalMaterial = false;

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(InteractTraceRadius),
		QueryParams
	);

	if (bDrawInteractTraceDebug)
	{
		/*
		// Draw from camera to actual trace END (not hit point) so the line
		// always shows full length regardless of what was hit
		DrawDebugLine(GetWorld(), TraceStart, TraceEnd, bHit ? FColor::Green : FColor::Red, false, 0.f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), bHit ? OutHit.ImpactPoint : TraceEnd, InteractTraceRadius, 12, bHit ? FColor::Green : FColor::Red, false, 0.f);
		*/
	}

	if (!bHit || !OutHit.GetActor()) return nullptr;

	AActor* HitActor = OutHit.GetActor();

	if (!HitActor->GetClass()->ImplementsInterface(UInteractable::StaticClass())) return nullptr;

	if (!IInteractable::Execute_CanInteract(HitActor, const_cast<AShooterGameCharacter*>(this)))
		return nullptr;

	return HitActor;
}

void AShooterGameCharacter::UpdateInteractFocus()
{
	// Do not update focus while downed or dead
	if (DownedComp && !DownedComp->IsAlive()) 
	{
		SetCurrentPromptActor(nullptr);
		return;
	}

	FHitResult HitResult;
	AActor* TraceResult = FindBestInteractableInView(HitResult);

	AActor* BestCandidate = TraceResult ? TraceResult : ResolveBestInteractionCandidate();
	SetCurrentPromptActor(BestCandidate);
}


AActor* AShooterGameCharacter::ResolveBestInteractionCandidate() const
{
	// Weapon overlap takes priority over ammo overlap — mirrors existing equip logic
	if (OverlappingWeapon) return OverlappingWeapon;
	if (OverlappingAmmoPickup) return OverlappingAmmoPickup;
	return nullptr;
}

void AShooterGameCharacter::SetCurrentPromptActor(AActor* NewPromptActor)
{
	if (CurrentPromptActor == NewPromptActor) return;

	// --- Unhighlight the previous focus if it was a general interactable ---
	if (CurrentHighlightedActor &&
		CurrentHighlightedActor->GetClass()->ImplementsInterface(UHighlightable::StaticClass()))
	{
		IHighlightable::Execute_UnHighlight(CurrentHighlightedActor);
		CurrentHighlightedActor = nullptr;
	}

	CurrentPromptActor = NewPromptActor;

	if (!CurrentPromptActor)
	{
		ClearCurrentPromptWidget();
		return;
	}

	// --- Highlight if the new actor supports it ---
	if (CurrentPromptActor->GetClass()->ImplementsInterface(UHighlightable::StaticClass()))
	{
		IHighlightable::Execute_Highlight(CurrentPromptActor);
		CurrentHighlightedActor = CurrentPromptActor;
	}

	RefreshCurrentPromptWidget();
}

void AShooterGameCharacter::RefreshCurrentPromptWidget()
{
	if (!CurrentPromptActor) return;

	// Resolve prompt text:
	// If the actor implements IInteractable, ask it for a custom prompt.
	// If not (e.g. overlapping weapon/ammo before they implement the interface),
	// fall back to a generic string so the widget always has something to show.
	FText PromptText;

	if (CurrentPromptActor->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
	{
		PromptText = IInteractable::Execute_GetInteractPromptText(CurrentPromptActor, this);
	}
	else if (OverlappingWeapon && CurrentPromptActor == OverlappingWeapon)
	{
		PromptText = FText::FromString(TEXT("Pick Up Weapon"));
	}
	else if (OverlappingAmmoPickup && CurrentPromptActor == OverlappingAmmoPickup)
	{
		PromptText = FText::FromString(TEXT("Collect Ammo"));
	}
	else
	{
		PromptText = FText::FromString(TEXT("Interact"));
	}

	InteractPromptWidgetInstance->SetPromptText(PromptText);
	InteractPromptWidgetInstance->SetVisibility(ESlateVisibility::Visible);
}

void AShooterGameCharacter::ClearCurrentPromptWidget()
{
	if (!InteractPromptWidgetInstance) return;

	InteractPromptWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
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


void AShooterGameCharacter::SetOverlappingAmmoPickup(AAmmoPickup* AmmoPickup)
{
	if (OverlappingAmmoPickup)
	{
		OverlappingAmmoPickup->ShowPickupWidget(false);
	}
	OverlappingAmmoPickup = AmmoPickup;
	if (IsLocallyControlled())
	{
		if (OverlappingAmmoPickup)
		{
			OverlappingAmmoPickup->ShowPickupWidget(true);
		}
	}
}

void AShooterGameCharacter::OnRep_OverlappingAmmoPickup(AAmmoPickup* LastAmmoPickup)
{
	if (OverlappingAmmoPickup)
	{
		OverlappingAmmoPickup->ShowPickupWidget(true);
	}
	if (LastAmmoPickup)
	{
		LastAmmoPickup->ShowPickupWidget(false);
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
	
	if (!OverlappingWeapon && OverlappingAmmoPickup)
    {
        if (HasAuthority())
        {
            ServerCollectAmmo();
        }
        else
        {
            ServerCollectAmmo();
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

void AShooterGameCharacter::ServerCollectAmmo_Implementation()
{
	if (!OverlappingAmmoPickup) return;

	UCombatComponent* CombatComp = GetCombat();
	if (!CombatComp) return;

	const FMagazine GrantedMag = OverlappingAmmoPickup->GetGrantedMagazine();

	UE_LOG(LogTemp, Warning,
		TEXT("ServerCollectAmmo — Granting mag: %d rounds, AmmoType: %d"),
		GrantedMag.CurrentRounds, (int32)GrantedMag.AmmoType);

	CombatComp->PickupMagazine(GrantedMag);
	OverlappingAmmoPickup->Destroy();

	// Verify inventory state after pickup
	if (UInventoryComponent* Inv = GetInventory())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ServerCollectAmmo — Inventory now has %d slots used"),
			Inv->GetUsedMagazineSlots());
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


void AShooterGameCharacter::PrimaryInteractButtonPressed()
{
	if (DownedComp && !DownedComp->IsAlive()) return;
	if (bInteractionAnimationRequested) return;

	ServerPrimaryInteract();
}

void AShooterGameCharacter::ServerPrimaryInteract_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] Called on server by %s"), *GetName());

	// --- Try focused general interactable first ---
	FHitResult HitResult;
	AActor* InteractTarget = FindBestInteractableInView(HitResult);

	if (InteractTarget)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ServerPrimaryInteract] Trace hit: %s"), *InteractTarget->GetName());

		if (!InteractTarget->GetClass()->ImplementsInterface(UInteractable::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] Actor does not implement IInteractable"));
			return;
		}

		if (!IInteractable::Execute_CanInteract(InteractTarget, this))
		{
			UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] CanInteract returned false"));
			return;
		}

		UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] Executing Interact on %s"), *InteractTarget->GetName());

		IInteractable::Execute_Interact(InteractTarget, this);
		
		ClientPlayInteractionMontage();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] No trace target — checking overlaps"));

	// --- Fallback: overlapping weapon ---
	if (OverlappingWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] Equipping overlapping weapon"));
		if (Combat)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		return;
	}

	// --- Fallback: overlapping ammo ---
	if (OverlappingAmmoPickup)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ServerPrimaryInteract] Collecting overlapping ammo"));
		ServerCollectAmmo();
	}
}


void AShooterGameCharacter::AnimNotify_InteractionFinished()
{
	StopInteractionAnimation();
}



bool AShooterGameCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void AShooterGameCharacter::ToggleAim()
{
	if (Combat)
	{
		const bool bNewAiming = !Combat->bAiming;
		Combat->SetAiming(bNewAiming);
		SetOrientationForAiming(bNewAiming);
	}
}

void AShooterGameCharacter::SetOrientationForAiming(bool bAiming)
{
	if (bAiming)
	{
		// Body locks to camera yaw — weapon aligns with reticle
		bUseControllerRotationYaw = true;
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		// Body freely turns toward movement direction
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void AShooterGameCharacter::SwapShoulder()
{
	bRightShoulder = !bRightShoulder;

	// Choose the correct target Y offset based on shoulder and aim state
	const bool bCurrentlyAiming = (Combat && Combat->bAiming);

	if (bRightShoulder)
	{
		TargetShoulderOffsetY = bCurrentlyAiming ? ADSRightShoulderOffsetY : RightShoulderOffsetY;
	}
	else
	{
		TargetShoulderOffsetY = bCurrentlyAiming ? ADSLeftShoulderOffsetY : LeftShoulderOffsetY;
	}
}


void AShooterGameCharacter::GoProneButtonPressed()
{
	// Cannot go prone while downed
	if (DownedComp && !DownedComp->IsAlive()) return;

	const bool bNewProne = !bIsProne;

	if (HasAuthority())
	{
		ServerSetProne_Implementation(bNewProne);
	}
	else
	{
		// Apply locally for immediate client feedback
		bIsProne = bNewProne;
		ApplyProneState(bNewProne);
		ServerSetProne(bNewProne);
	}
}

void AShooterGameCharacter::ServerSetProne_Implementation(bool bNewProne)
{
	bIsProne = bNewProne;
	ApplyProneState(bNewProne);
}

void AShooterGameCharacter::OnRep_IsProne()
{
	// Fires on simulated proxies when bIsProne replicates
	ApplyProneState(bIsProne);
}

void AShooterGameCharacter::ApplyProneState(bool bProne)
{
	if (!GetCharacterMovement()) return;

	if (bProne)
	{
		// Slow movement significantly
		GetCharacterMovement()->MaxWalkSpeed = ProneWalkSpeed;

		// Shrink capsule to near-ground height
		GetCapsuleComponent()->SetCapsuleHalfHeight(ProneCapsuleHalfHeight);

		// If crouched, stand before going prone
		if (bIsCrouched)
		{
			UnCrouch();
		}

		UE_LOG(LogShooterGameCharacter, Warning,
			TEXT("AShooterGameCharacter::ApplyProneState — PRONE"));
	}
	else
	{
		// Restore to run speed — sprint/walk tiers are enforced in Tick
		GetCharacterMovement()->MaxWalkSpeed = RunSpeed;

		// Restore capsule height
		GetCapsuleComponent()->SetCapsuleHalfHeight(StandingCapsuleHalfHeight);

		UE_LOG(LogShooterGameCharacter, Warning,
			TEXT("AShooterGameCharacter::ApplyProneState — STANDING"));
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

void AShooterGameCharacter::ToggleSuppressor_Input(const FInputActionValue& Value)
{
	if (!Combat) return;

	// Play the suppressor montage on this character if one is assigned.
	// Leave SuppressorMontage = nullptr in BP until you have the animation.
	if (SuppressorMontage)
	{
		PlayAnimMontage(SuppressorMontage);
	}

	// Delay the actual suppressor toggle to sync with the animation midpoint.
	// Using 0.0f for now (instant) — tune this once you have a real montage.
	const float ToggleDelay = SuppressorMontage ? 0.5f : 0.0f;

	if (ToggleDelay > 0.f)
	{
		FTimerHandle SuppressorToggleTimer;
		GetWorldTimerManager().SetTimer(
			SuppressorToggleTimer,
			[this]() { if (Combat) Combat->ToggleSuppressor(); },
			ToggleDelay,
			false
		);
	}
	else
	{
		Combat->ToggleSuppressor();
	}
}

void AShooterGameCharacter::PlayReloadMontage()
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance || !Combat) return;

	UAnimMontage* MontageToPlay = nullptr;

	// Config-driven lookup — primary path
	if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(this))
	{
		switch (Combat->GetReloadType())
		{
		case EReloadType::Empty:  MontageToPlay = Cfg->TPReloadEmpty; break;
		case EReloadType::Quick:  MontageToPlay = Cfg->TPReloadQuick; break;
		default:                  MontageToPlay = Cfg->TPReload;      break;
		}
	}

	// Fallback to legacy character properties if config slot is not yet assigned
	if (!MontageToPlay)
	{
		switch (Combat->GetReloadType())
		{
		case EReloadType::Empty:  MontageToPlay = Montage_Reload_Empty; break;
		case EReloadType::Quick:  MontageToPlay = Montage_Reload_Quick; break;
		default:                  MontageToPlay = Montage_Reload;       break;
		}
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[Reload] PlayReloadMontage — AnimInst: %d, MontageToPlay: %d, ReloadType: %d"),
		AnimInstance != nullptr, MontageToPlay != nullptr, (int32)Combat->GetReloadType());

	if (MontageToPlay && !AnimInstance->Montage_IsPlaying(MontageToPlay))
	{
		AnimInstance->Montage_Play(MontageToPlay);
	}
}

void AShooterGameCharacter::PlayInteractionMontage()
{
	UAnimMontage* MontageToPlay = GetInteractionMontageForCurrentStance();
	
	if (!MontageToPlay)
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->Montage_Play(MontageToPlay);
	}
}

UAnimMontage* AShooterGameCharacter::GetInteractionMontageForCurrentStance() const
{
	if (!Combat)
	{
		return InteractionMontage_Unarmed;
	}

	switch (Combat->GetPlayerWeaponStance())
	{
	case EPlayerWeaponStance::EPWS_Unarmed:
		return InteractionMontage_Unarmed;

	case EPlayerWeaponStance::EPWS_Pistol:
		return InteractionMontage_Pistol ? InteractionMontage_Pistol : InteractionMontage_Unarmed;

	case EPlayerWeaponStance::EPWS_Rifle:
	case EPlayerWeaponStance::EPWS_Shotgun:
	case EPlayerWeaponStance::EPWS_Other:
		return InteractionMontage_Rifle ? InteractionMontage_Rifle : InteractionMontage_Unarmed;

	default:
		return InteractionMontage_Unarmed;
	}
}


void AShooterGameCharacter::StartInteractionAnimation()
{
	if (bInteractionAnimationRequested) return;
	SetInteractionAnimationRequested(true);

	// Notify combat state
	if (Combat)
	{
		Combat->SetCombatAction(ECombatAction::Interacting);
	}

	if (IsLocallyControlled()) PlayInteractionMontage();
}

void AShooterGameCharacter::StopInteractionAnimation()
{
	SetInteractionAnimationRequested(false);

	// Clear combat state
	if (Combat)
	{
		Combat->SetCombatAction(ECombatAction::None);
	}
}

void AShooterGameCharacter::ClientPlayInteractionMontage_Implementation()
{
	PlayInteractionMontage();
}

void AShooterGameCharacter::SetInteractionAnimationRequested(bool bRequested)
{
	bInteractionAnimationRequested = bRequested;
}


bool AShooterGameCharacter::IsReloadAnimationPlaying() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return false;

	// Check config montages first
	if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(const_cast<AShooterGameCharacter*>(this)))
	{
		if ((Cfg->TPReload      && AnimInstance->Montage_IsPlaying(Cfg->TPReload))
		 || (Cfg->TPReloadEmpty && AnimInstance->Montage_IsPlaying(Cfg->TPReloadEmpty))
		 || (Cfg->TPReloadQuick && AnimInstance->Montage_IsPlaying(Cfg->TPReloadQuick)))
		{
			return true;
		}
	}

	// Legacy fallback properties
	return (Montage_Reload      && AnimInstance->Montage_IsPlaying(Montage_Reload))
		|| (Montage_Reload_Empty && AnimInstance->Montage_IsPlaying(Montage_Reload_Empty))
		|| (Montage_Reload_Quick && AnimInstance->Montage_IsPlaying(Montage_Reload_Quick));
}

bool AShooterGameCharacter::IsInteractionAnimationPlaying() const
{
	const UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	UAnimMontage* MontageToCheck = GetInteractionMontageForCurrentStance();

	return AnimInstance && MontageToCheck && AnimInstance->Montage_IsPlaying(MontageToCheck);
}


void AShooterGameCharacter::PlayEquipMontage()
{
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) return;

    UAnimMontage* MontageToPlay = nullptr;
    if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(this))
    {
        MontageToPlay = Cfg->TPEquip;
    }

    if (MontageToPlay && !AnimInstance->Montage_IsPlaying(MontageToPlay))
    {
        AnimInstance->Montage_Play(MontageToPlay);
    }
}

void AShooterGameCharacter::PlayFireModeMontage()
{
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) return;
	
	UE_LOG(LogTemp, Warning, TEXT("[M6] AnimInstance class: %s"), *AnimInstance->GetClass()->GetName());

    UAnimMontage* MontageToPlay = nullptr;
    if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(this))
    {
        MontageToPlay = Cfg->TPFireMode;
    }

    // Fallback to legacy property
    if (!MontageToPlay)
    {
        MontageToPlay = Montage_FireModeSwitch;
    }

    if (MontageToPlay && !AnimInstance->Montage_IsPlaying(MontageToPlay))
    {
    	UE_LOG(LogTemp, Warning, TEXT("[M6] PlayFireMontage — playing: %s, length: %.2f"),
		*MontageToPlay->GetName(),
		MontageToPlay->GetPlayLength());
    	AnimInstance->Montage_Play(MontageToPlay);
        AnimInstance->Montage_Play(MontageToPlay);
    }
}

void AShooterGameCharacter::PlayMagCheckMontage()
{
    // Input binding deferred — called manually or via future IA_MagCheck binding.
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) return;

    UAnimMontage* MontageToPlay = nullptr;
    if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(this))
    {
        MontageToPlay = Cfg->TPMagCheck;
    }

    // Fallback to legacy property
    if (!MontageToPlay)
    {
        MontageToPlay = Montage_MagCheck;
    }

    if (MontageToPlay && !AnimInstance->Montage_IsPlaying(MontageToPlay))
    {
        AnimInstance->Montage_Play(MontageToPlay);
    }
}

void AShooterGameCharacter::PlayInspectMontage()
{
    // Input binding deferred — called manually or via future IA_Inspect binding.
    UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
    if (!AnimInstance) return;

    const AWeapon* Weapon    = GetEquippedWeapon();
    const bool bMagEmpty     = Weapon && Weapon->GetMagRounds() == 0;

    UAnimMontage* MontageToPlay = nullptr;
    if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(this))
    {
        MontageToPlay = bMagEmpty ? Cfg->TPInspectEmpty : Cfg->TPInspect;
        // If InspectEmpty is not set, fall back to Inspect regardless of mag state
        if (!MontageToPlay) MontageToPlay = Cfg->TPInspect;
    }

    if (MontageToPlay && !AnimInstance->Montage_IsPlaying(MontageToPlay))
    {
        AnimInstance->Montage_Play(MontageToPlay);
    }
}


void AShooterGameCharacter::PlayFireMontage(bool bAiming)
{
	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance) return;

	const AWeapon* Weapon    = GetEquippedWeapon();
	const bool bMagEmpty     = Weapon && Weapon->GetMagRounds() == 0;

	// Config-driven lookup — primary path
	UAnimMontage* MontageToPlay = nullptr;
	if (const UWeaponConfig* Cfg = GetConfigForEquippedWeapon(this))
	{
		if (bMagEmpty)
			MontageToPlay = Cfg->TPFireEmpty;
		else if (bAiming && Cfg->TPFireADS)
			MontageToPlay = Cfg->TPFireADS;
		else
			MontageToPlay = Cfg->TPFire;
	}

	// Fallback to legacy character properties if config slot is not yet assigned
	if (!MontageToPlay)
	{
		MontageToPlay = bMagEmpty ? Montage_Fire_Empty : Montage_Fire;
	}

	if (MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[M6] AnimInstance class: %s"),
		*AnimInstance->GetClass()->GetName());
		AnimInstance->Montage_Play(MontageToPlay);
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



// -----------------------------------------------------------------------
// Loadout / Appearance internal handlers
// Temporary stubs — Step 6 moves loadout response logic to CombatComponent.
// Appearance logic will be expanded when the cosmetic system is built out.
// -----------------------------------------------------------------------

void AShooterGameCharacter::OnLoadoutChanged_Internal(const FLoadoutData& NewLoadout)
{
	if (!HasAuthority()) return;

	UInventoryComponent* InventoryComp = GetInventory();
	if (!InventoryComp) return;

	InventoryComp->ClearAllMagazines();

	for (const FLoadoutSlot& Slot : NewLoadout.Slots)
	{
		if (!Slot.IsOccupied() || Slot.MagazineCount <= 0) continue;

		UClass* ResolvedClass = Slot.ItemClass.LoadSynchronous();
		if (!ResolvedClass) continue;

		AWeapon* WeaponCDO = Cast<AWeapon>(ResolvedClass->GetDefaultObject());
		if (!WeaponCDO) continue;

		UWeaponConfig* Config = WeaponCDO->GetWeaponConfig();
		if (!Config) continue;

		const EAmmoType AmmoType = WeaponCDO->GetSupportedAmmoType();
		const int32     Capacity = Config->MagazineCapacity;

		if (Capacity <= 0) continue;

		for (int32 i = 0; i < Slot.MagazineCount; i++)
		{
			InventoryComp->AddMagazine(FMagazine::MakeFull(AmmoType, Capacity));
		}
	}
}

void AShooterGameCharacter::OnAppearanceChanged_Internal(const FCharacterAppearance& NewAppearance)
{
	// Future: drive mesh/material swap here or in a dedicated AppearanceComponent
	UE_LOG(LogShooterGameCharacter, Log,
		TEXT("[%s] Appearance updated — SkinID: %d, HelmetID: %d, BackpackID: %d"),
		HasAuthority() ? TEXT("Server") : TEXT("Client"),
		NewAppearance.MeshSkinID,
		NewAppearance.HelmetID,
		NewAppearance.BackpackID);
}


void AShooterGameCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AShooterPlayerState* PS = GetPlayerState<AShooterPlayerState>())
	{
		// Seed PlayerState with DefaultLoadout if it has no saved data yet
		if (!PS->HasSavedLoadout())
		{
			PS->SetSavedLoadout(DefaultLoadout);
		}
		PS->PushLoadoutToCharacter(this);
	}
}

