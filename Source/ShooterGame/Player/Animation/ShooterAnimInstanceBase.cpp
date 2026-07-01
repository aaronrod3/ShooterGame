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

    // Lazy init — character/component may not have been ready at NativeInitializeAnimation
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
    UpdateWeaponConfigData();
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

    
    const float GripTarget       = (CurrentGrip != EWeaponGrip::None) ? 1.f : 0.f;
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

    static const FName LeftGripSocketName  = FName("SOCKET_Grip");
    static const FName RightGripSocketName = FName("SOCKET_Grip_R");
    static const FName LeftBoneName  = FName("hand_l");
    static const FName RightBoneName = FName("hand_r");

    USkeletalMeshComponent* CharMesh = ShooterGameCharacter->GetMesh();
    if (!CharMesh || !Weapon->GetWeaponMesh()->DoesSocketExist(LeftGripSocketName))
    {
        bLeftHandOnWeapon = bGripOverrideActive ? bLeftHandOnWeaponOverride : false;
        return;
    }

    FTransform SocketTransform = Weapon->GetWeaponMesh()->GetSocketTransform(
        LeftGripSocketName, RTS_World
    );

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

    FVector WorldDebug = CharMesh->GetBoneTransform(
        CharMesh->GetBoneIndex(RightBoneName)
    ).TransformPosition(OutPosition);
    DrawDebugSphere(GetWorld(), WorldDebug, 3.f, 8, FColor::Green, false, -1.f);

    bLeftHandOnWeapon = bGripOverrideActive ? bLeftHandOnWeaponOverride : true;

    UE_LOG(LogTemp, Warning, TEXT("[IK] COMPLETE — bLeftHandOnWeapon: %d | bGripOverride: %d"),
        (int32)bLeftHandOnWeapon,
        (int32)bGripOverrideActive
    );

    // -----------------------------------------------------------------------
    // Right hand — mirrors the left-hand computation above, but reads
    // SOCKET_Grip_R and expresses the result in hand_l's bone space.
    // hand_l is unaffected by the right arm's own FABRIK chain
    // (clavicle_r -> hand_r -> ik_hand_r), so it's a safe, non-circular
    // reference — exactly how hand_r serves as the left hand's reference.
    // No bRightHandOnWeapon flag: the AnimGraph gates the right-arm FABRIK
    // alpha with bWeaponEquipped directly, since the right hand never
    // detaches from the weapon mid-montage.
    // -----------------------------------------------------------------------
    if (Weapon->GetWeaponMesh()->DoesSocketExist(RightGripSocketName))
    {
        FTransform RightSocketTransform = Weapon->GetWeaponMesh()->GetSocketTransform(
            RightGripSocketName, RTS_World
        );

        FVector RightOutPosition;
        FRotator RightOutRotation;
        CharMesh->TransformToBoneSpace(
            LeftBoneName,
            RightSocketTransform.GetLocation(),
            RightSocketTransform.Rotator(),
            RightOutPosition,
            RightOutRotation
        );

        RightHandTransform.SetLocation(RightOutPosition);
        RightHandTransform.SetRotation(FQuat(RightOutRotation));
        RightHandTransform.SetScale3D(FVector::OneVector);
    }
}

void UShooterAnimInstanceBase::UpdateWeaponConfigData()
{
    if (ShooterGameCharacter)
    {
        const AWeapon* Weapon = ShooterGameCharacter->GetEquippedWeapon();
        WeaponConfig = Weapon ? Weapon->GetWeaponConfig() : nullptr;
    }
}