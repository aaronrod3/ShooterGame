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
    
    
    //
    // TEMP LEGACY COMPATIBILITY
    //
        // -----------------------------------------------------------------------
    // Temporary compatibility aliases
    // Keep these while migrating existing gameplay code to the BP-style names.
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Animation Blueprints|Weapon")
    TSubclassOf<UAnimInstance> ABPWeapon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Animation Blueprints|Weapon")
    TSubclassOf<UAnimInstance> ABPMagazine;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPFire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPFireADS;
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPFireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPReload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPEquip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPInspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPInspectEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPMagCheck;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPGrenadeThrowQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TSoftObjectPtr<UAnimMontage> TPRandomization;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPEquip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPFireSemi;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPFireADS;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPFireAuto;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPFireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPReload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPInspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPInspectEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPMagCheck;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPGrenadeThrowQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPJumpFull;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TSoftObjectPtr<UAnimMontage> FPRandomization;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FVector ADSLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FRotator ADSRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FRotator RecoilRotationPerShot = FRotator(2.f, 0.5f, 0.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FVector RecoilTranslationPerShot = FVector(-1.f, 0.f, 0.f);
    
    
    
    
    
    /****************************************/
    
    /** Starting cosmetic ammo count for magazine depletion animation. Default 0 = unset. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 TotalAmmoCount = 0;
    
    
    
    
    

    // -----------------------------------------------------------------------
    // Mesh References
    // -----------------------------------------------------------------------

    /** Main skeletal mesh — the weapon receiver / frame. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<USkeletalMesh> MeshReceiver;

    /** Skeletal magazine mesh (animated, used for cosmetic magazine actors). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<USkeletalMesh> MeshMagazineSK;

    /** Static magazine mesh (used for dropped / physics magazine prop). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshMagazine;

    /** Live round mesh shown in magazine bullet sockets. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshBullet;

    /** Chambered / ejected casing mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshBulletCasing;

    /** Handguard static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshHandguard;

    /** Slide static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshSlide;
    
    /** Scope static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshScope;

    /** Front iron sight static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshSightFront;

    /** Rear iron sight static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshSightRear;

    /** Laser attachment static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshLaserAttachment;

    /** Vertical foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshGripVertical;

    /** Angled foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshGripAngled;

    /** Silencer / suppressor static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Weapon")
    TSoftObjectPtr<UStaticMesh> MeshSilencer;

    /** First-person character skeletal mesh (arms + weapon). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Character")
    TSoftObjectPtr<USkeletalMesh> FP_Mesh;

    /** Third-person character skeletal mesh (full body with weapon pose). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes|Character")
    TSoftObjectPtr<USkeletalMesh> TP_Mesh;

    // -----------------------------------------------------------------------
    // AnimBP Class References
    // -----------------------------------------------------------------------

    /** AnimBP class to assign to the weapon receiver skeletal mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Animation Blueprints|Weapon")
    TSubclassOf<UAnimInstance> ABP_Weapon;

    /** AnimBP class to assign to the cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Animation Blueprints|Weapon")
    TSubclassOf<UAnimInstance> ABP_Magazine;

    // -----------------------------------------------------------------------
    // Socket Names
    // -----------------------------------------------------------------------

    /** Socket on the character mesh where the weapon actor attaches. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketGunAttachment = NAME_None;

    /** Socket on the receiver for the main cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketMagazineAttachment = NAME_None;

    /** Socket on the receiver for the reserve cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketMagazineReserveAttachment = NAME_None;

    /** Socket on the receiver for the laser attachment actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketLaserAttachment = NAME_None;

    /** Socket on the receiver for the foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketGripAttachment = NAME_None;

    /** Socket on the receiver for casing ejection spawn. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketCasingEject = NAME_None;

    /** Socket on the laser attachment mesh for the beam origin trace. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketLaserStart = NAME_None;

    /** Socket on the receiver for the handguard static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketHandguard = NAME_None;

    /** Socket on the receiver for muzzle flash VFX spawn / line trace origin. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketMuzzle = NAME_None;
    
    /** Socket on the receiver for muzzle flash VFX spawn / line trace origin. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketSlide = NAME_None;

    /** Socket on the receiver for the chambered round mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketBulletChambered = NAME_None;

    /** Socket on the receiver for the jammed casing mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketCasingJammed = NAME_None;

    /** Socket on the receiver for the scope static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketScope = NAME_None;

    /** Socket on the receiver for the front iron sight. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketSightFront = NAME_None;

    /** Socket on the receiver for the rear iron sight. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketSightRear = NAME_None;

    /** Socket on the receiver / scope for the gun / scope camera. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketGunCamera = NAME_None;

    /** Socket on the character mesh for helmet bodycam attachment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketHelmetCamera = NAME_None;

    /**
     * Socket on the character mesh for chest bodycam attachment.
     * Left as NAME_None deliberately — helmet and chest cam are explicit per weapon.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Sockets")
    FName SocketChestCamera = NAME_None;

    // -----------------------------------------------------------------------
    // First-Person Character Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_FireSemi;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_Reload;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_ReloadEmpty;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_ReloadQuick;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_InspectEmpty;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_MagCheck;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_Inspect;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_GrenadeThrowQuick;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_JumpFull;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_Equip;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_FireMode;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_FireAuto;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_FireEmpty;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TSoftObjectPtr<UAnimMontage> FP_Randomization;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Melee;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Malfunctions;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Interactions;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|First Person")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_Healing;
    
    
    // -----------------------------------------------------------------------
    // Third-Person Character Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_Inspect;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_Reload;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_ReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_ReloadQuick;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_InspectEmpty;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_Fire;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_MagCheck;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_GrenadeThrowQuick;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Melee;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_Equip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_FireMode;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_FireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Malfunctions;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Interactions;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_Healing;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Character|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_Randomization;

    // -----------------------------------------------------------------------
    // First-Person Weapon Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Reload;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Inspect;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Fire;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_ReloadEmpty;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_ReloadQuick;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_MagCheck;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TArray<TSoftObjectPtr<UAnimMontage>> FP_WEP_Malfunctions;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|First Person")
    TSoftObjectPtr<UAnimMontage> FP_WEP_Equip;
    
    // -----------------------------------------------------------------------
    // Third-Person Weapon Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_MagCheck;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Reload;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Inspect;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Fire;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_ReloadEmpty;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_ReloadQuick;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TSoftObjectPtr<UAnimMontage> TP_WEP_Equip;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Montages|Weapon|Third Person")
    TArray<TSoftObjectPtr<UAnimMontage>> TP_WEP_Malfunctions;

    // -----------------------------------------------------------------------
    // Locomotion — Blend Spaces
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Blendspaces Locomotion")
    TSoftObjectPtr<UBlendSpace> FP_Locomotion_Standing;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Blendspaces Locomotion")
    TSoftObjectPtr<UBlendSpace> FP_Locomotion_Crouching;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Blendspaces Locomotion")
    TSoftObjectPtr<UBlendSpace> FP_Locomotion_Aiming;

    
    
    
   

    // -----------------------------------------------------------------------
    // Locomotion — First-Person Poses & Transitions
    // -----------------------------------------------------------------------

    

    
    
    
   

    // -----------------------------------------------------------------------
    // Locomotion — Third-Person Poses & Transitions
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_IdlePose;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_AimPose;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_AimPoseCanted;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_IdlePoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_IdlePoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_AimPoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Poses")
    TSoftObjectPtr<UAnimSequence> TP_AimPoseGripVertical;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Loops")
    TSoftObjectPtr<UAnimSequence> TP_IdleLoop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Transitions")
    TSoftObjectPtr<UAnimSequence> TP_Transition_AimStart;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|Third Person|Transitions")
    TSoftObjectPtr<UAnimSequence> TP_Transition_AimEnd;
    
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Transitions")
    TSoftObjectPtr<UAnimSequence> FP_Transition_WalkEnd;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Transitions")
    TSoftObjectPtr<UAnimSequence> FP_Transition_RunEnd;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Transitions")
    TSoftObjectPtr<UAnimSequence> FP_Transition_CrouchStart;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Transitions")
    TSoftObjectPtr<UAnimSequence> FP_Transition_CrouchEnd;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Loops")
    TSoftObjectPtr<UAnimSequence> FP_RunLoop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Loops")
    TSoftObjectPtr<UAnimSequence> FP_SprintLoop;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Poses")
    TSoftObjectPtr<UAnimSequence> FP_IdlePose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Poses")
    TSoftObjectPtr<UAnimSequence> FP_AimPose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Poses")
    TSoftObjectPtr<UAnimSequence> FP_IdlePoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Poses")
    TSoftObjectPtr<UAnimSequence> FP_IdlePoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Poses")
    TSoftObjectPtr<UAnimSequence> FP_AimPoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Character|First Person|Poses")
    TSoftObjectPtr<UAnimSequence> FP_AimPoseGripVertical;

    // -----------------------------------------------------------------------
    // Weapon Pose / State Sequences
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Weapon|Poses")
    TSoftObjectPtr<UAnimSequence> WEP_ReferencePose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Weapon|Poses")
    TSoftObjectPtr<UAnimSequence> WEP_FireModeStates;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Sequences|Weapon|Magazine")
    TSoftObjectPtr<UAnimSequence> WEP_MagazineDepletion;

    // -----------------------------------------------------------------------
    // Sounds
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio|Sound Cues|Weapon")
    TSoftObjectPtr<USoundBase> SoundCue_WEP_MagDrop;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio|Sound Cues|Weapon")
    TSoftObjectPtr<USoundBase> SoundCue_WEP_CasingEject;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio|Sound Cues|Weapon")
    TSoftObjectPtr<USoundBase> SoundCue_WEP_AimIn;
    
    
    // -----------------------------------------------------------------------
    // Offsets
    // -----------------------------------------------------------------------

    /** ADS procedural spring offset. Mirrors BP_TFA_BaseConfig.OffsetAimDownSights. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Procedural Offsets")
    FTransform OffsetAimDownSights = FTransform::Identity;

    /** Crouch procedural spring offset. Mirrors BP_TFA_BaseConfig.OffsetCrouch. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Procedural Offsets")
    FTransform OffsetCrouch = FTransform::Identity;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Procedural Offsets")
    FTransform OffsetCantedAim = FTransform::Identity;
    

    // -----------------------------------------------------------------------
    // Cosmetic Values
    // -----------------------------------------------------------------------

    /** Bullet socket prefix. Sockets are named Bullet_01, Bullet_02, etc. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Names|Prefixes")
    FString PrefixBulletSocket = TEXT("Bullet_");

    

    // -----------------------------------------------------------------------
    // Grip & Capacity (project extensions — not in Infima base config)
    // -----------------------------------------------------------------------

    /** Left-hand grip pose. Read by UCombatComponent on equip. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    EWeaponGrip WeaponGrip = EWeaponGrip::None;

    /** Authoritative magazine capacity used by LoadoutComponent and CombatComponent. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets",
        meta = (ClampMin = "1"))
    int32 MagazineCapacity = 30;

    // -----------------------------------------------------------------------
    // UPrimaryDataAsset interface
    // -----------------------------------------------------------------------
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};