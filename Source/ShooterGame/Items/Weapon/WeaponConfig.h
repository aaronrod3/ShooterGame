// Source/ShooterGame/Items/Weapon/WeaponConfig.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShooterGame/Types/CombatTypes.h"
#include "WeaponConfig.generated.h"

/**
 * UWeaponConfig
 *
 * Primary data asset that is the single source of truth for a weapon's
 * meshes, sockets, montages, AnimBP references, offsets, sounds, and
 * cosmetic ammo count.  Mirrors Infima's BPTFABaseConfig design.
 *
 * Both AWeapon (presentation assembly) and AShooterGameCharacter
 * (equip flow, camera, offsets) read from this asset.
 * Gameplay logic (ammo economy, damage) stays in AWeapon and UCombatComponent.
 */
UCLASS(BlueprintType)
class SHOOTERGAME_API UWeaponConfig : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    // -----------------------------------------------------------------------
    // Mesh References
    // -----------------------------------------------------------------------

    /** Main skeletal mesh — the rifle receiver / frame. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<USkeletalMesh> MeshReceiver;

    /** Skeletal magazine mesh (animated, used for cosmetic magazine actors). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<USkeletalMesh> MeshMagazineSK;

    /** Static magazine mesh (used for dropped/physics magazine prop). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshMagazine;

    /** Chambered or ejected bullet mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshBulletCasing;

    /** Live round mesh shown in magazine bullet sockets. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshBullet;

    /** Handguard static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshHandguard;

    /** Scope static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshScope;

    /** Front iron sight static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshSightFront;

    /** Rear iron sight static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshSightRear;

    /** Silencer / suppressor static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshSilencer;

    /** Laser attachment static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshLaserAttachment;

    /** Vertical foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshGripVertical;

    /** Angled foregrip static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<UStaticMesh> MeshGripAngled;

    /** Third-person character skeletal mesh (full body with weapon pose). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<USkeletalMesh> TPMesh;

    /** First-person character skeletal mesh (arms only). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Meshes")
    TObjectPtr<USkeletalMesh> FPMesh;

    // -----------------------------------------------------------------------
    // AnimBP References
    // -----------------------------------------------------------------------

    /** AnimBP class for the weapon receiver skeletal mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation")
    TSubclassOf<UAnimInstance> ABPWeapon;

    /** AnimBP class for the cosmetic magazine actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation")
    TSubclassOf<UAnimInstance> ABPMagazine;

    // -----------------------------------------------------------------------
    // Socket Names
    // -----------------------------------------------------------------------

    /** Socket on character mesh where the weapon actor attaches. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketGunAttachment = NAME_None;

    /** Socket on receiver for the main cosmetic magazine. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketMagazineAttachment = NAME_None;

    /** Socket on receiver for the reserve cosmetic magazine. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketMagazineReserveAttachment = NAME_None;

    /** Socket on receiver for the grip mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketGripAttachment = NAME_None;

    /** Socket on receiver for the laser attachment actor. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketLaserAttachment = NAME_None;

    /** Socket on the laser attachment mesh for the beam origin trace. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketLaserStart = NAME_None;

    /** Socket on receiver for casing ejection spawn. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketCasingEject = NAME_None;

    /** Socket on receiver for the handguard static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketHandguard = NAME_None;

    /** Socket on receiver for muzzle flash VFX spawn. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketMuzzle = NAME_None;

    /** Socket on receiver for the scope static mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketScope = NAME_None;

    /** Socket on receiver for the front sight. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketSightFront = NAME_None;

    /** Socket on receiver for the rear sight. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketSightRear = NAME_None;

    /** Socket on receiver for the chambered round mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketBulletChambered = NAME_None;

    /** Socket on receiver for the jammed casing mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketCasingJammed = NAME_None;

    /** Socket on receiver for the gun/scope camera attachment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketGunCamera = NAME_None;

    /** Socket on character mesh for helmet bodycam attachment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketHelmetCamera = NAME_None;

    /** Socket on character mesh for chest bodycam attachment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketChestCamera = NAME_None;

    /** Socket on the left hand for IK targeting. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sockets")
    FName SocketLeftHandIK = NAME_None;

    // -----------------------------------------------------------------------
    // Third-Person Character Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPEquip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPFire;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPFireADS;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPFireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPReload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPInspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPInspectEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPMagCheck;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPGrenadeThrowQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TObjectPtr<UAnimMontage> TPRandomization;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TObjectPtr<UAnimMontage>> TPMelee;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TObjectPtr<UAnimMontage>> TPMalfunctions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TObjectPtr<UAnimMontage>> TPInteractions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TP")
    TArray<TObjectPtr<UAnimMontage>> TPHealing;

    // -----------------------------------------------------------------------
    // First-Person Character Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPEquip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPFireSemi;
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPFireADS;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPFireAuto;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPFireEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPReload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPInspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPInspectEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPMagCheck;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPFireMode;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPGrenadeThrowQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPJumpFull;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TObjectPtr<UAnimMontage> FPRandomization;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TObjectPtr<UAnimMontage>> FPMelee;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TObjectPtr<UAnimMontage>> FPMalfunctions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TObjectPtr<UAnimMontage>> FPInteractions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FP")
    TArray<TObjectPtr<UAnimMontage>> FPHealing;

    // -----------------------------------------------------------------------
    // Third-Person Weapon Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPEquip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPFire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPReload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPInspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|TPWeapon")
    TObjectPtr<UAnimMontage> TPWEPMagCheck;

    // -----------------------------------------------------------------------
    // First-Person Weapon Montages
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPEquip;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPFire;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPReload;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPReloadEmpty;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPReloadQuick;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPInspect;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Montages|FPWeapon")
    TObjectPtr<UAnimMontage> FPWEPMagCheck;

    // -----------------------------------------------------------------------
    // Locomotion / Pose Assets  (filled per-weapon in the data asset)
    // -----------------------------------------------------------------------

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation|Poses")
    TObjectPtr<UAnimSequence> TPIdlePose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation|Poses")
    TObjectPtr<UAnimSequence> TPAimPose;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation|Poses")
    TObjectPtr<UAnimSequence> TPIdlePoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation|Poses")
    TObjectPtr<UAnimSequence> TPAimPoseGripAngled;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation|Poses")
    TObjectPtr<UAnimSequence> TPIdlePoseGripVertical;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Animation|Poses")
    TObjectPtr<UAnimSequence> TPAimPoseGripVertical;

    // -----------------------------------------------------------------------
    // Procedural Offsets
    // -----------------------------------------------------------------------

    /**
     * Transform applied to the procedural ADS spring when the player aims.
     * Drives the character mesh offset while ADS is active.
     * Matches BPTFABaseConfig.OffsetAimDownSights.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FTransform OffsetAimDownSights = FTransform::Identity;

    /**
     * Transform applied to the procedural crouch spring when the player crouches.
     * Matches BPTFABaseConfig.OffsetCrouch.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Offsets")
    FTransform OffsetCrouch = FTransform::Identity;

    // -----------------------------------------------------------------------
    // Sounds
    // -----------------------------------------------------------------------

    /** Sound cue played when a magazine is dropped. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sounds")
    TObjectPtr<USoundBase> SoundCueWEPMagDrop;

    /** Sound cue played when a casing is ejected and impacts a surface. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sounds")
    TObjectPtr<USoundBase> SoundCueWEPCasingEject;

    /** Sound cue played when the player enters ADS. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Sounds")
    TObjectPtr<USoundBase> SoundCueWEPAimIn;

    // -----------------------------------------------------------------------
    // Cosmetic Values
    // -----------------------------------------------------------------------

    /**
     * Starting cosmetic ammo count used by the weapon's visual magazine system.
     * This is NOT authoritative ammo — authoritative ammo lives in AWeapon /
     * UCombatComponent.  This value drives magazine depletion animation only.
     * Matches BPTFABaseConfig.TotalAmmoCount.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Cosmetics",
        meta = (ClampMin = "0"))
    int32 TotalAmmoCount = 30;

    /**
     * Prefix used to find bullet sockets on the magazine skeleton.
     * Defaults to "Bullet" to match the Infima convention.
     * Matches BPTFABaseConfig.PrefixBulletSocket.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Cosmetics")
    FString PrefixBulletSocket = TEXT("Bullet");
    
    
    // -----------------------------------------------------------------------
    // Grip
    // -----------------------------------------------------------------------

    /**
     * Which left-hand grip pose this weapon uses.
     * Read by UCombatComponent::EquipWeapon to set CurrentGrip on equip.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Grip")
    EWeaponGrip WeaponGrip = EWeaponGrip::Default;

    // -----------------------------------------------------------------------
    // ADS Offsets
    // -----------------------------------------------------------------------

    /**
     * World-space location offset applied to the camera/character when fully
     * aimed down sights. Drives ADSLocationTarget on UCombatComponent.
     * Matches Infima's BPTFABaseConfig ADS location offset convention.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|ADS")
    FVector ADSLocationOffset = FVector::ZeroVector;

    /**
     * Rotation offset applied when fully aimed down sights.
     * Drives ADSRotationTarget on UCombatComponent.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|ADS")
    FRotator ADSRotationOffset = FRotator::ZeroRotator;

    // -----------------------------------------------------------------------
    // Recoil Offsets
    // -----------------------------------------------------------------------

    /**
     * Rotation target for procedural recoil per shot.
     * X = Pitch kick (upward), Y = Yaw drift, Z = Roll.
     * Interpolated by UCombatComponent each tick and read by the anim instance.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Recoil")
    FRotator RecoilRotationPerShot = FRotator(2.f, 0.5f, 0.f);

    /**
     * Translation kick applied per shot (camera spring offset).
     * Typically a small negative X (push back) value.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Recoil")
    FVector RecoilTranslationPerShot = FVector(-1.f, 0.f, 0.f);
    
    

    // -----------------------------------------------------------------------
    // UPrimaryDataAsset interface
    // -----------------------------------------------------------------------
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
    
    /**
     * Authoritative magazine capacity for this weapon.
     * Used by LoadoutComponent to grant starting magazines on spawn,
     * and by CombatComponent when constructing a fresh magazine during reload.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config|Ammo",
        meta = (ClampMin = "1"))
    int32 MagazineCapacity = 30;
    
    
};