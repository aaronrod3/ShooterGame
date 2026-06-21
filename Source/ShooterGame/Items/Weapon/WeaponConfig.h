// Source/ShooterGame/Items/Weapon/WeaponConfig.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "Sound/SoundBase.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "ShooterGame/Types/CombatTypes.h"
#include "WeaponConfig.generated.h"

/**
 * UWeaponConfig
 *
 * Primary data asset — the single source of truth for all weapon-specific
 * mesh references, socket names, animation assets, sounds, offsets, and
 * cosmetic values consumed by the character, weapon actor, magazine actor,
 * and AnimBPs.
 *
 * Mirrors Infima Games BP_TFA_BaseConfig field-for-field.
 * All asset references are TSoftObjectPtr so the Asset Manager can stream
 * weapon configs without eagerly loading every referenced asset.
 *
 * Gameplay logic (ammo economy, damage, fire rate) lives in AWeapon /
 * UCombatComponent — NOT here.
 */
UCLASS(BlueprintType)
class SHOOTERGAME_API UWeaponConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Mesh References
    // -----------------------------------------------------------------------

    /** Main skeletal mesh — the weapon receiver / frame. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<USkeletalMesh> MeshReceiver;

    /** Skeletal magazine mesh (animated, used for cosmetic magazine actors). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<USkeletalMesh> MeshMagazineSK;

    /** Static magazine mesh (used for dropped / physics magazine prop). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshMagazine;

    /** Live round mesh shown in magazine bullet sockets. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshBullet;

    /** Chambered / ejected casing mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshBulletCasing;

    /** Handguard static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshHandguard;

    /** Scope static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshScope;

    /** Front iron sight static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshSightFront;

    /** Rear iron sight static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshSightRear;

    /** Laser attachment static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshLaserAttachment;

    /** Vertical foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshGripVertical;

    /** Angled foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshGripAngled;

    /** Silencer / suppressor static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<UStaticMesh> MeshSilencer;

    /** First-person character skeletal mesh (arms + weapon). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<USkeletalMesh> FP_Mesh;

    /** Third-person character skeletal mesh (full body with weapon pose). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSoftObjectPtr<USkeletalMesh> TP_Mesh;

    // -----------------------------------------------------------------------
    // AnimBP Class References
    // -----------------------------------------------------------------------

    /** AnimBP class to assign to the weapon receiver skeletal mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSubclassOf<UAnimInstance> ABP_Weapon;

    /** AnimBP class to assign to the cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TSubclassOf<UAnimInstance> ABP_Magazine;

    // -----------------------------------------------------------------------
    // Socket Names
    // -----------------------------------------------------------------------

    /** Socket on the character mesh where the weapon actor attaches. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketGunAttachment = NAME_None;

    /** Socket on the receiver for the main cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketMagazineAttachment = NAME_None;

    /** Socket on the receiver for the reserve cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketMagazineReserveAttachment = NAME_None;

    /** Socket on the receiver for the laser attachment actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketLaserAttachment = NAME_None;

    /** Socket on the receiver for the foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketGripAttachment = NAME_None;

    /** Socket on the receiver for casing ejection spawn. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketCasingEject = NAME_None;

    /** Socket on the laser attachment mesh for the beam origin trace. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketLaserStart = NAME_None;

    /** Socket on the receiver for the handguard static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketHandguard = NAME_None;

    /** Socket on the receiver for muzzle flash VFX spawn / line trace origin. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketMuzzle = NAME_None;

    /** Socket on the receiver for the chambered round mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketBulletChambered = NAME_None;

    /** Socket on the receiver for the jammed casing mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketCasingJammed = NAME_None;

    /** Socket on the receiver for the scope static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketScope = NAME_None;

    /** Socket on the receiver for the front iron sight. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketSightFront = NAME_None;

    /** Socket on the receiver for the rear iron sight. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketSightRear = NAME_None;

    /** Socket on the receiver / scope for the gun / scope camera. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketGunCamera = NAME_None;

    /** Socket on the character mesh for helmet bodycam attachment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketHelmetCamera = NAME_None;

    /**
     * Socket on the character mesh for chest bodycam attachment.
     * Left as NAME_None deliberately — helmet and chest cam are explicit per weapon.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketChestCamera = NAME_None;

    // -----------------------------------------------------------------------
    // First-Person Character Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_Equip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_FireSemi;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_FireAuto;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_FireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_Reload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_ReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_ReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_Inspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_InspectEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_MagCheck;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_GrenadeThrowQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_FireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_JumpFull;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FP_Randomization;

    // -----------------------------------------------------------------------
    // Third-Person Character Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_Equip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_Fire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_FireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_Reload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_ReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_ReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_Inspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_InspectEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_MagCheck;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_GrenadeThrowQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_FireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TP_Randomization;

    // -----------------------------------------------------------------------
    // First-Person Weapon Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Equip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Fire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Reload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_ReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_ReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Inspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> FP_WEP_MagCheck;

    // -----------------------------------------------------------------------
    // Third-Person Weapon Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Equip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Fire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Reload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_ReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_ReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Inspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TSoftObjectPtr<UAnimMontage> TP_WEP_MagCheck;

    // -----------------------------------------------------------------------
    // Animation Arrays
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Melee;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Melee;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Malfunctions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Malfunctions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_WEP_Malfunctions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|Weapon")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_WEP_Malfunctions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Interactions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Interactions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Healing;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Healing;

    // -----------------------------------------------------------------------
    // Locomotion — Blend Spaces
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UBlendSpace> FP_Locomotion_Standing;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UBlendSpace> FP_Locomotion_Crouching;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UBlendSpace> FP_Locomotion_Aiming;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_RunLoop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_SprintLoop;

    // -----------------------------------------------------------------------
    // Locomotion — First-Person Poses & Transitions
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_IdlePose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_AimPose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_IdlePoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_IdlePoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_AimPoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_AimPoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_Transition_WalkEnd;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_Transition_RunEnd;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_Transition_CrouchStart;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> FP_Transition_CrouchEnd;

    // -----------------------------------------------------------------------
    // Locomotion — Third-Person Poses & Transitions
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_IdleLoop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_IdlePose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_AimPose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_IdlePoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_IdlePoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_AimPoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_AimPoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_Transition_AimStart;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> TP_Transition_AimEnd;

    // -----------------------------------------------------------------------
    // Weapon Pose / State Sequences
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> WEP_ReferencePose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> WEP_FireModeStates;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Locomotion")
    TSoftObjectPtr<UAnimSequence> WEP_MagazineDepletion;

    // -----------------------------------------------------------------------
    // Offsets
    // -----------------------------------------------------------------------

    /** ADS procedural spring offset. Mirrors BP_TFA_BaseConfig.OffsetAimDownSights. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FTransform OffsetAimDownSights = FTransform::Identity;

    /** Crouch procedural spring offset. Mirrors BP_TFA_BaseConfig.OffsetCrouch. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FTransform OffsetCrouch = FTransform::Identity;

    // -----------------------------------------------------------------------
    // Sounds
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sounds")
    TSoftObjectPtr<USoundBase> SoundCue_WEP_MagDrop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sounds")
    TSoftObjectPtr<USoundBase> SoundCue_WEP_CasingEject;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sounds")
    TSoftObjectPtr<USoundBase> SoundCue_WEP_AimIn;

    // -----------------------------------------------------------------------
    // Cosmetic Values
    // -----------------------------------------------------------------------

    /** Bullet socket prefix. Sockets are named Bullet_01, Bullet_02, etc. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FString PrefixBulletSocket = TEXT("Bullet_");

    /** Starting cosmetic ammo count for magazine depletion animation. Default 0 = unset. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets",
        meta = (ClampMin = "0"))
    int32 TotalAmmoCount = 0;

    // -----------------------------------------------------------------------
    // Grip & Capacity (project extensions — not in Infima base config)
    // -----------------------------------------------------------------------

    /** Left-hand grip pose. Read by UCombatComponent on equip. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    EWeaponGrip WeaponGrip = EWeaponGrip::Default;

    /** Authoritative magazine capacity used by LoadoutComponent and CombatComponent. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets",
        meta = (ClampMin = "1"))
    int32 MagazineCapacity = 30;

    // -----------------------------------------------------------------------
    // UPrimaryDataAsset interface
    // -----------------------------------------------------------------------
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};