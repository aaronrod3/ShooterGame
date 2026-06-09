#include "ShooterAnimInstanceBase.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/Items/Weapon/Weapon.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UShooterAnimInstanceBase::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    ShooterGameCharacter = Cast<AShooterGameCharacter>(GetOwningActor());
    if (ShooterGameCharacter)
    {
        CombatComponent = ShooterGameCharacter->GetCombat();
    }
}

void UShooterAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

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
}

void UShooterAnimInstanceBase::UpdateAimData()
{
    bIsAiming      = ShooterGameCharacter->IsAiming();
    AimOffset_Yaw  = ShooterGameCharacter->GetAimOffset_Yaw();
    TurningInPlace = ShooterGameCharacter->GetTurningInPlace();

    const FRotator ControlRot = ShooterGameCharacter->GetControlRotation();
    AimOffset_Pitch = ControlRot.Pitch > 180.f
        ? ControlRot.Pitch - 360.f
        : ControlRot.Pitch;
}

void UShooterAnimInstanceBase::UpdateCombatData()
{
    if (!CombatComponent) return;

    CurrentAction     = CombatComponent->GetCombatAction();
    CurrentReloadType = CombatComponent->GetReloadType();
    CurrentGrip       = CombatComponent->GetCurrentGrip();
    WeaponStance      = CombatComponent->GetPlayerWeaponStance();
    bIsBusy           = CombatComponent->IsBusy();
    bIsAimingBlocked  = CombatComponent->IsAimingBlocked();
    bWeaponEquipped   = (ShooterGameCharacter->GetEquippedWeapon() != nullptr);

    bIsReloading   = (CurrentAction == ECombatAction::Reloading)
                     || CombatComponent->IsReloadPendingLocal();
    bIsInteracting = (CurrentAction == ECombatAction::Interacting)
                     || ShooterGameCharacter->IsInteractionAnimationRequested();
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

    const FName LeftSocketName  = ShooterGameCharacter->GetLeftHandIKSocketName();
    const FName RightBoneName   = ShooterGameCharacter->GetRightHandIKBoneName();
    const FVector LocOffset     = ShooterGameCharacter->GetLeftHandIKLocationOffset();
    const FRotator RotOffset    = ShooterGameCharacter->GetLeftHandIKRotationOffset();

    USkeletalMeshComponent* CharMesh = ShooterGameCharacter->GetMesh();
    if (!CharMesh || !Weapon->GetWeaponMesh()->DoesSocketExist(LeftSocketName))
    {
        bLeftHandOnWeapon = false;
        return;
    }

    // Get the socket in world space and apply designer offsets
    FTransform SocketTransform = Weapon->GetWeaponMesh()->GetSocketTransform(
        LeftSocketName, RTS_World
    );
    SocketTransform.AddToTranslation(LocOffset);
    SocketTransform.ConcatenateRotation(RotOffset.Quaternion());

    // Convert to right-hand bone space — this is what the Two Bone IK node expects
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

    bLeftHandOnWeapon = true;
}