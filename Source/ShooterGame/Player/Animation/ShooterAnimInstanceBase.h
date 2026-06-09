#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ShooterGame/Types/CombatTypes.h"
#include "ShooterGame/Types/PlayerWeaponStance.h"
#include "ShooterGame/Types/TurningInPlace.h"
#include "ShooterAnimInstanceBase.generated.h"

class AShooterGameCharacter;
class UCombatComponent;

/**
 * Shared base for all ShooterGame anim instances (TP and FP).
 * Owns the data that both perspectives need: movement, combat action
 * state, weapon stance, and left-hand IK inputs.
 * Does NOT own any montage slots or pose logic — those belong to the
 * derived TP/FP classes.
 */
UCLASS()
class SHOOTERGAME_API UShooterAnimInstanceBase : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UFUNCTION()
    void AnimNotify_InteractionFinished();
    
    
protected:
    // -----------------------------------------------------------------------
    // Cached pointers — set once in NativeInitializeAnimation
    // -----------------------------------------------------------------------
    UPROPERTY(BlueprintReadOnly, Category = "Anim|Cache")
    TObjectPtr<AShooterGameCharacter> ShooterGameCharacter;

    UPROPERTY()
    TObjectPtr<UCombatComponent> CombatComponent;

    // -----------------------------------------------------------------------
    // Movement state
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float Speed = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    bool bIsInAir = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    bool bIsAccelerating = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    bool bIsCrouched = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    bool bIsProne = false;

    /** Per-frame yaw delta used for locomotion blendspaces. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float YawOffset = 0.f;

    /** Smoothed version of YawOffset — fed into lean blendspaces. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float Lean = 0.f;

    /** Strafe direction angle — fed into directional blendspaces. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float Direction = 0.f;

    // -----------------------------------------------------------------------
    // Aim / turn state
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    bool bIsAiming = false;

    /** Yaw delta between actor and control rotation. Fed into AimOffset. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    float AimOffset_Yaw = 0.f;

    /** Pitch of the control rotation. Fed into AimOffset. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    float AimOffset_Pitch = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    ETurningInPlace TurningInPlace = ETurningInPlace::ETIP_NotTurning;

    // -----------------------------------------------------------------------
    // Combat action state (from CombatComponent)
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    ECombatAction CurrentAction = ECombatAction::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    EReloadType CurrentReloadType = EReloadType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    EWeaponGrip CurrentGrip = EWeaponGrip::Default;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    EPlayerWeaponStance WeaponStance = EPlayerWeaponStance::EPWS_Unarmed;

    /** True while a weapon is equipped. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bWeaponEquipped = false;

    /** True while any action is blocking inputs (reload, interact, etc.). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsBusy = false;

    /** Convenience bool — true when CurrentAction == Reloading. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsReloading = false;

    /** Convenience bool — true when CurrentAction == Interacting. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsInteracting = false;

    /** True when a montage notify state is blocking ADS entry. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsAimingBlocked = false;

    // -----------------------------------------------------------------------
    // Left-hand IK inputs (fed from character socket config)
    // -----------------------------------------------------------------------

    /** Component-space transform of the left-hand IK target this frame. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    FTransform LeftHandTransform = FTransform::Identity;

    /** True when the left hand should be IK-pinned to the weapon. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    bool bLeftHandOnWeapon = false;

private:
    float LastYaw = 0.f;

    void UpdateMovementData();
    void UpdateAimData();
    void UpdateCombatData();
    void UpdateIKData();
};