#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ShooterGame/Types/CombatTypes.h"
#include "ShooterGame/Types/PlayerWeaponStance.h"
#include "ShooterGame/Types/TurningInPlace.h"
#include "ShooterGame/Player/Animation/ShooterAnimStateInterface.h"
#include "ShooterAnimInstanceBase.generated.h"

class AShooterGameCharacter;
class UCombatComponent;


// ---------------------------------------------------------------------------
// Stance enum — used by both FP and TP AnimBPs to drive locomotion layers.
// ---------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EShooterStance : uint8
{
    Standing  UMETA(DisplayName = "Standing"),
    Crouching UMETA(DisplayName = "Crouching"),
    Aiming    UMETA(DisplayName = "Aiming"),
};


UCLASS()
class SHOOTERGAME_API UShooterAnimInstanceBase : public UAnimInstance, public IShooterAnimStateInterface
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UFUNCTION()
    void AnimNotify_InteractionFinished();
    
    // IShooterAnimStateInterface
    virtual void UpdateLeftHandGrip_Implementation(bool bIsLeftHandOnWeapon, float BlendSpeed) override;
    virtual void SetAimingBlocked_Implementation(bool bBlocked) override;
    virtual void OnAnimStateMessage_Implementation(FName MessageTag, float Value) override;
    
    /** Called by ANS_LeftHandGrip::NotifyEnd to hand control back to UpdateIKData. */
    FORCEINLINE void ClearGripOverride()
    {
        bGripOverrideActive    = false;
        GripBlendSpeedOverride = 0.f;
    }

protected:
    // -----------------------------------------------------------------------
    // Cached pointers
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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float YawOffset = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float Lean = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement")
    float Direction = 0.f;

    // -----------------------------------------------------------------------
    // Locomotion tier flags — Phase 1 Infima bridge contract
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement|Tiers")
    bool bIsWalking = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement|Tiers")
    bool bIsRunning = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement|Tiers")
    bool bIsSprinting = false;

    /**
     * Local-space 2D movement vector for Infima-style blend spaces.
     * X = forward/backward (cm/s), Y = right/left strafe (cm/s).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Movement|Tiers")
    FVector2D InputMoveVector = FVector2D::ZeroVector;

    // -----------------------------------------------------------------------
    // Speed threshold config
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Movement|Thresholds")
    float IdleSpeedThreshold = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Movement|Thresholds")
    float RunSpeedThreshold = 150.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Movement|Thresholds")
    float SprintSpeedThreshold = 250.f;

    // -----------------------------------------------------------------------
    // Aim / turn state
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    bool bIsAiming = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    float AimOffset_Yaw = 0.f;

    /**
     * Normalized control pitch for aim offset vertical axis.
     * Range: [-90, 90]. UE wrap-around resolved in character Tick.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    float AimOffset_Pitch = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Aim")
    ETurningInPlace TurningInPlace = ETurningInPlace::ETIP_NotTurning;

    // -----------------------------------------------------------------------
    // Stance — drives FP and TP locomotion state machines
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Stance")
    EShooterStance CurrentStance = EShooterStance::Standing;
    
    
    // -----------------------------------------------------------------------
    // Combat action state
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    ECombatAction CurrentAction = ECombatAction::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    EReloadType CurrentReloadType = EReloadType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    EWeaponGrip CurrentGrip = EWeaponGrip::Default;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    EPlayerWeaponStance WeaponStance = EPlayerWeaponStance::EPWS_Unarmed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bWeaponEquipped = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsBusy = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsReloading = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsInteracting = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsAimingBlocked = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bInCombatState = false;
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bHighReady = false;

    // -----------------------------------------------------------------------
    // Left-hand IK inputs
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    FTransform LeftHandTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    bool bLeftHandOnWeapon = false;

    // -----------------------------------------------------------------------
    // Procedural transform contract — Phase 1 Infima bridge
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Procedural")
    FTransform CrouchTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Procedural")
    FTransform AimDownSightsTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Procedural")
    FTransform RecoilTransform = FTransform::Identity;

    // -----------------------------------------------------------------------
    // Grip blend alpha — Phase 1 Infima bridge
    // -----------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    float CurrentGripAlpha = 0.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Anim|Combat")
    float GripInterpSpeed = 10.f;
    
    /** True while ANS_LeftHandGrip is actively overriding left-hand IK. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    bool bGripOverrideActive = false;

    /**
     * Notify-driven override for bLeftHandOnWeapon.
     * When bGripOverrideActive is true, UpdateIKData result is ignored
     * and this value is used instead.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    bool bLeftHandOnWeaponOverride = false;

    /**
     * Per-message blend speed override.
     * When > 0, CurrentGripAlpha interpolation uses this instead of GripInterpSpeed.
     * Reset to 0 when notify ends.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|IK")
    float GripBlendSpeedOverride = 0.f;

    /**
     * Notify-driven ADS block — set by ANS_BlockADS.
     * Takes precedence over CombatComponent::bIsAimingBlocked for the AnimInstance.
     * CombatComponent flag is the authoritative server value; this is a local
     * AnimInstance shadow for the current montage window only.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Combat")
    bool bIsAimingBlockedLocal = false;

private:
    float LastYaw = 0.f;

    void UpdateMovementData();
    void UpdateAimData();
    void UpdateCombatData();
    void UpdateIKData();
};