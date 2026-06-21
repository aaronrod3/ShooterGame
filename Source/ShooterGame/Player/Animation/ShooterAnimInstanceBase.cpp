#include "ShooterAnimInstanceBase.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterAnimStateInterface.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterGame/Framework/ShooterGame.h"
#include "KismetAnimationLibrary.h"

void UShooterAnimInstanceBase::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    USkeletalMeshComponent* OwningMesh = GetSkelMeshComponent();
    UE_LOG(LogTemp, Warning, TEXT("[AnimInit] NativeInitializeAnimation — Class: %s | Mesh: %s"),
        *GetClass()->GetName(),
        OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            41, 10.f, FColor::Cyan,
            FString::Printf(TEXT("[AnimInit] %s on mesh: %s"),
                *GetClass()->GetName(),
                OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"))
        );
    }

    ShooterGameCharacter = Cast<AShooterGameCharacter>(GetOwningActor());
    if (ShooterGameCharacter)
    {
        CombatComponent = ShooterGameCharacter->GetCombat();
    }

    UE_LOG(LogTemp, Warning, TEXT("[AnimInit] %s — Character: %s | CombatComponent: %s"),
        *GetClass()->GetName(),
        ShooterGameCharacter ? TEXT("valid") : TEXT("NULL"),
        CombatComponent ? TEXT("valid") : TEXT("NULL"));
}

void UShooterAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    
    USkeletalMeshComponent* OwningMesh = GetSkelMeshComponent();
    UE_LOG(LogTemp, Warning, TEXT("[AnimInit] NativeInitializeAnimation — Class: %s | Mesh: %s"),
        *GetClass()->GetName(),
        OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"));

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(
            41, 10.f, FColor::Cyan,
            FString::Printf(TEXT("[AnimInit] %s on mesh: %s"),
                *GetClass()->GetName(),
                OwningMesh ? *OwningMesh->GetName() : TEXT("NULL"))
        );
    }

    ShooterGameCharacter = Cast<AShooterGameCharacter>(GetOwningActor());
    if (ShooterGameCharacter)
    {
        CombatComponent = ShooterGameCharacter->GetCombat();
    }

    UE_LOG(LogTemp, Warning, TEXT("[AnimInit] %s — Character: %s | CombatComponent: %s"),
        *GetClass()->GetName(),
        ShooterGameCharacter ? TEXT("valid") : TEXT("NULL"),
        CombatComponent ? TEXT("valid") : TEXT("NULL"));

    if (!ShooterGameCharacter)
    {
        ShooterGameCharacter = Cast<AShooterGameCharacter>(GetOwningActor());
    }
    if (ShooterGameCharacter && !CombatComponent)
    {
        CombatComponent = ShooterGameCharacter->GetCombat();
    }
    if (!ShooterGameCharacter) return;

    UpdateMovementData();
    UpdateAimData();
    UpdateCombatData();
    UpdateIKData();
}


void UShooterAnimInstanceBase::AnimNotify_InteractionFinished()
{
    if (ShooterGameCharacter)
    {
        ShooterGameCharacter->StopInteractionAnimation();
    }
}


// -----------------------------------------------------------------------
// IShooterAnimStateInterface — _Implementation bodies
// -----------------------------------------------------------------------

void UShooterAnimInstanceBase::UpdateLeftHandGrip_Implementation(
    bool bIsLeftHandOnWeapon, float BlendSpeed)
{
    bGripOverrideActive         = true;
    bLeftHandOnWeaponOverride   = bIsLeftHandOnWeapon;
    GripBlendSpeedOverride      = (BlendSpeed > 0.f) ? BlendSpeed : GripInterpSpeed;

    UE_LOG(LogShooterGame, Verbose,
        TEXT("UShooterAnimInstanceBase::UpdateLeftHandGrip — OnWeapon=%d BlendSpeed=%.1f"),
        bIsLeftHandOnWeapon, GripBlendSpeedOverride);
}

void UShooterAnimInstanceBase::SetAimingBlocked_Implementation(bool bBlocked)
{
    bIsAimingBlockedLocal = bBlocked;

    UE_LOG(LogShooterGame, Verbose,
        TEXT("UShooterAnimInstanceBase::SetAimingBlocked — Blocked=%d"),
        bBlocked);
}

void UShooterAnimInstanceBase::OnAnimStateMessage_Implementation(
    FName MessageTag, float Value)
{
    // Default implementation is a no-op pass-through.
    // Blueprint AnimBPs can override this for tag-driven responses.
    UE_LOG(LogShooterGame, Verbose,
        TEXT("UShooterAnimInstanceBase::OnAnimStateMessage — Tag=%s Value=%.2f"),
        *MessageTag.ToString(), Value);
}



void UShooterAnimInstanceBase::UpdateMovementData()
{
    const UCharacterMovementComponent* MoveComp = ShooterGameCharacter->GetCharacterMovement();
    if (!MoveComp) return;

    Speed           = ShooterGameCharacter->GetVelocity().Size2D();
    bIsInAir        = MoveComp->IsFalling();
    bIsAccelerating = MoveComp->GetCurrentAcceleration().SizeSquared() > 0.f;
    bIsCrouched     = ShooterGameCharacter->bIsCrouched;
    bIsProne        = ShooterGameCharacter->GetIsProne();

    const float CurrentYaw = ShooterGameCharacter->GetActorRotation().Yaw;
    const float RawYawDelta = FMath::FindDeltaAngleDegrees(LastYaw, CurrentYaw);
    YawOffset = (GetWorld()->GetDeltaSeconds() > 0.f)
        ? RawYawDelta / GetWorld()->GetDeltaSeconds()
        : 0.f;
    Lean      = FMath::FInterpTo(Lean, YawOffset, GetWorld()->GetDeltaSeconds(), 6.f);
    LastYaw   = CurrentYaw;

    Direction = UKismetAnimationLibrary::CalculateDirection(
        ShooterGameCharacter->GetVelocity(),
        ShooterGameCharacter->GetActorRotation()
    );

    // -----------------------------------------------------------------------
    // Phase 1 — locomotion tier flags
    // -----------------------------------------------------------------------
    bIsSprinting = ShooterGameCharacter->IsSprinting();
    bIsRunning   = !bIsSprinting && (Speed >= RunSpeedThreshold);
    bIsWalking   = !bIsSprinting && !bIsRunning && (Speed >= IdleSpeedThreshold);

    // 2D local-space move vector for Infima-style blend spaces
    InputMoveVector = ShooterGameCharacter->GetAnimationInputMoveVector();
}

void UShooterAnimInstanceBase::UpdateAimData()
{
    bIsAiming       = ShooterGameCharacter->IsAiming();
    AimOffset_Yaw   = ShooterGameCharacter->GetAimOffset_Yaw();
    AimOffset_Pitch = ShooterGameCharacter->GetAimOffset_Pitch();
    TurningInPlace  = ShooterGameCharacter->GetTurningInPlace();

    // -----------------------------------------------------------------------
    // Stance derivation — Aiming > Crouching > Standing
    // Placed here because bIsAiming and bIsCrouched are both resolved above
    // and inside UpdateMovementData() respectively.
    // -----------------------------------------------------------------------
    if (bIsAiming)        CurrentStance = EShooterStance::Aiming;
    else if (bIsCrouched) CurrentStance = EShooterStance::Crouching;
    else                  CurrentStance = EShooterStance::Standing;
}

void UShooterAnimInstanceBase::UpdateCombatData()
{
    // Lazy re-cache — CombatComponent may have been null at NativeInitializeAnimation
    if (!CombatComponent && ShooterGameCharacter)
    {
        CombatComponent = ShooterGameCharacter->GetCombat();
    }
    if (!CombatComponent) return;

    CurrentAction       = CombatComponent->GetCombatAction();
    CurrentReloadType   = CombatComponent->GetReloadType();
    CurrentGrip         = CombatComponent->GetCurrentGrip();
    WeaponStance        = CombatComponent->GetPlayerWeaponStance();
    bIsBusy             = CombatComponent->IsBusy();
    bIsAimingBlocked    = bIsAimingBlockedLocal || CombatComponent->IsAimingBlocked();
    bWeaponEquipped     = (ShooterGameCharacter->GetEquippedWeapon() != nullptr);
    bInCombatState      = CombatComponent->GetInCombatState();
    bHighReady          = CombatComponent->GetHighReady();

    bIsReloading        = (CurrentAction == ECombatAction::Reloading)|| CombatComponent->IsReloadPendingLocal();
    bIsInteracting      = (CurrentAction == ECombatAction::Interacting) || ShooterGameCharacter->IsInteractionAnimationRequested();

    
    const float GripTarget       = (CurrentGrip != EWeaponGrip::Default) ? 1.f : 0.f;
    const float ActiveBlendSpeed = (GripBlendSpeedOverride > 0.f) ? GripBlendSpeedOverride : GripInterpSpeed;
    CurrentGripAlpha = FMath::FInterpTo(
        CurrentGripAlpha,
        GripTarget,
        GetWorld()->GetDeltaSeconds(),
        ActiveBlendSpeed
    );

    // -----------------------------------------------------------------------
    // Phase 1 — procedural transform reads from character
    // Authored offset targets are zero/identity until tuning phase.
    // -----------------------------------------------------------------------
    CrouchTransform       = ShooterGameCharacter->GetCrouchTransform();
    AimDownSightsTransform = ShooterGameCharacter->GetAimDownSightsTransform();
    RecoilTransform       = ShooterGameCharacter->GetRecoilTransform();
    
    if (GEngine)
    {
        const bool bIsTP = GetClass()->GetName().Contains(TEXT("TP"));
        const int32 MsgKey = bIsTP ? 21 : 25;  // separate keys so they don't overwrite each other
        const FColor MsgColor = bIsTP ? FColor::Yellow : FColor::Purple;

        FName StateName = NAME_None;
        if (bIsTP)
        {
            int32 MachineIndex = GetStateMachineIndex(FName("SM_TP_UpperBody"));
            if (MachineIndex != INDEX_NONE)
            {
                StateName = GetCurrentStateName(MachineIndex);
            }
        }

        GEngine->AddOnScreenDebugMessage(
            MsgKey, 0.f, MsgColor,
            FString::Printf(TEXT("[%s] bHighReady:%d | bInCombatState:%d | State:%s"),
                bIsTP ? TEXT("TP") : TEXT("FP"),
                (int32)bHighReady,
                (int32)bInCombatState,
                *StateName.ToString())
        );
    }
}

void UShooterAnimInstanceBase::UpdateIKData()
{
    if (!CombatComponent) return;

    AWeapon* Weapon = ShooterGameCharacter->GetEquippedWeapon();
    if (!Weapon || !Weapon->GetWeaponMesh())
    {
        bLeftHandOnWeapon = false;
        return;
    }

    const FName LeftSocketName = ShooterGameCharacter->GetLeftHandIKSocketName();
    const FName RightBoneName  = ShooterGameCharacter->GetRightHandIKBoneName();
    const FVector LocOffset    = ShooterGameCharacter->GetLeftHandIKLocationOffset();
    const FRotator RotOffset   = ShooterGameCharacter->GetLeftHandIKRotationOffset();

    USkeletalMeshComponent* CharMesh = ShooterGameCharacter->GetMesh();
    if (!CharMesh || !Weapon->GetWeaponMesh()->DoesSocketExist(LeftSocketName))
    {
        bLeftHandOnWeapon = bGripOverrideActive ? bLeftHandOnWeaponOverride : false;
        return;
    }

    FTransform SocketTransform = Weapon->GetWeaponMesh()->GetSocketTransform(
        LeftSocketName, RTS_World
    );
    SocketTransform.AddToTranslation(LocOffset);
    SocketTransform.ConcatenateRotation(RotOffset.Quaternion());

    FVector OutPosition;
    FRotator OutRotation;
    CharMesh->TransformToBoneSpace(
        RightBoneName,
        SocketTransform.GetLocation(),
        SocketTransform.Rotator(),
        OutPosition,
        OutRotation
    );

    LeftHandTransform.SetLocation(OutPosition);
    LeftHandTransform.SetRotation(FQuat(OutRotation));
    LeftHandTransform.SetScale3D(FVector::OneVector);

    bLeftHandOnWeapon = bGripOverrideActive ? bLeftHandOnWeaponOverride : true;
}