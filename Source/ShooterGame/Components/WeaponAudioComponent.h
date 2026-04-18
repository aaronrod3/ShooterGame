// Source/ShooterGame/Components/WeaponAudioComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "WeaponAudioComponent.generated.h"

/**
 * UWeaponAudioComponent
 *
 * Full weapon audio architecture for AWeapon.
 * Extended from the existing ShooterGame implementation to support:
 * - one-shot / loop / stop / tail
 * - suppressed variants
 * - dry fire
 * - reload rifle / pistol / bullet drop
 * - fire mode switch
 * - suppressor equip / unequip
 *
 * Multiplayer pattern:
 *   Public ForMultiplayer() call
 *     -> Server RPC
 *     -> NetMulticast RPC
 *     -> local Internal_ playback
 *
 * Important:
 * - This component handles playback only.
 * - UAudioPerceptionComponent remains the only gameplay noise emitter.
 * - Noise reporting stays in AWeapon::Fire() only.
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UWeaponAudioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWeaponAudioComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

    // -----------------------------------------------------------------------
    // UNSUPPRESSED Cue Sets
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_OneShot;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_Loop;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_Stop;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_Tail;

    // -----------------------------------------------------------------------
    // SUPPRESSED Cue Sets
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Suppressed")
    TArray<USoundCue*> Cues_OneShot_Suppressed;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Suppressed")
    TArray<USoundCue*> Cues_Loop_Suppressed;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Suppressed")
    TArray<USoundCue*> Cues_Stop_Suppressed;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Suppressed")
    TArray<USoundCue*> Cues_Tail_Suppressed;

    // -----------------------------------------------------------------------
    // EXTRA CUES
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Extra")
    TArray<USoundCue*> Cues_SwitchFireMode;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Extra")
    TArray<USoundCue*> Cues_EquipUnequipSuppressor;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Extra")
    TArray<USoundCue*> Cues_DryFire;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Reload")
    TArray<USoundCue*> Cues_ReloadRifle;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Reload")
    TArray<USoundCue*> Cues_ReloadPistol;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Reload")
    TArray<USoundCue*> Cues_ReloadBulletDrop;

    // -----------------------------------------------------------------------
    // Timing
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Offsets")
    TArray<float> OffsetLoopTimes;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Offsets")
    float OffsetOneShotTime = 0.04f;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Offsets")
    float CrossfadeOneShotLoopTime = 0.05f;

    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Offsets")
    float FadeOutDuration = 0.2f;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayGunshot_ForMultiplayer();

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayGunshotLoop_ForMultiplayer(float InCrossfadeTime = -1.f);

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void StopLoop_ForMultiplayer(float InFadeOutDuration = -1.f);

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayDryFire_ForMultiplayer();

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayReload_ForMultiplayer(bool bIsPistolClass);

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayReloadBulletDrop_ForMultiplayer();

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlaySwitchFireMode_ForMultiplayer();

    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayEquipUnequipSuppressor_ForMultiplayer();

protected:
    virtual void BeginPlay() override;

private:
    // -----------------------------------------------------------------------
    // Internal Audio Component Management
    // -----------------------------------------------------------------------

    UAudioComponent* CreateAudioComp(USoundCue* SoundCue);
    TArray<UAudioComponent*> CreateAudioComps(const TArray<USoundCue*>& SoundCues);
    void InitialiseAllSounds();

    // Pre-created audio comps
    UPROPERTY()
    TArray<UAudioComponent*> ACs_Start;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Loop;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Stop;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Tail;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Start_Suppressed;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Loop_Suppressed;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Stop_Suppressed;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_Tail_Suppressed;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_SwitchFireMode;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_EquipUnequipSuppressor;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_DryFire;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_ReloadRifle;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_ReloadPistol;

    UPROPERTY()
    TArray<UAudioComponent*> ACs_ReloadBulletDrop;

    bool bIsLoopPlaying = false;

    // -----------------------------------------------------------------------
    // Internal local play functions
    // -----------------------------------------------------------------------

    void Internal_PlayOneShot(bool bSuppressed);
    void Internal_PlayOneShotLoop(bool bSuppressed, float InCrossfadeTime);
    void Internal_StopLoop(bool bSuppressed, float InFadeOutDuration);
    void Internal_PlayDryFire();
    void Internal_PlayReload(bool bIsPistolClass);
    void Internal_PlayReloadBulletDrop();
    void Internal_PlaySwitchFireMode();
    void Internal_PlayEquipUnequipSuppressor();

    void PlayAudioComponentArray(TArray<UAudioComponent*>& AudioComps, float StartTime = 0.f);
    void FadeOutAudioComponentArray(TArray<UAudioComponent*>& AudioComps, float InFadeOutDuration);
    void PlayCueArrayAtOwnerLocation(const TArray<USoundCue*>& Cues);

    bool IsSuppressed() const;

    // -----------------------------------------------------------------------
    // Multiplayer RPCs
    // -----------------------------------------------------------------------

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayGunshot(bool bSuppressed);
    void Server_PlayGunshot_Implementation(bool bSuppressed);
    bool Server_PlayGunshot_Validate(bool bSuppressed);

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayGunshot(bool bSuppressed);
    void Multi_PlayGunshot_Implementation(bool bSuppressed);
    bool Multi_PlayGunshot_Validate(bool bSuppressed);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayGunshotLoop(bool bSuppressed, float InCrossfadeTime);
    void Server_PlayGunshotLoop_Implementation(bool bSuppressed, float InCrossfadeTime);
    bool Server_PlayGunshotLoop_Validate(bool bSuppressed, float InCrossfadeTime);

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayGunshotLoop(bool bSuppressed, float InCrossfadeTime);
    void Multi_PlayGunshotLoop_Implementation(bool bSuppressed, float InCrossfadeTime);
    bool Multi_PlayGunshotLoop_Validate(bool bSuppressed, float InCrossfadeTime);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_StopLoop(bool bSuppressed, float InFadeOutDuration);
    void Server_StopLoop_Implementation(bool bSuppressed, float InFadeOutDuration);
    bool Server_StopLoop_Validate(bool bSuppressed, float InFadeOutDuration);

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_StopLoop(bool bSuppressed, float InFadeOutDuration);
    void Multi_StopLoop_Implementation(bool bSuppressed, float InFadeOutDuration);
    bool Multi_StopLoop_Validate(bool bSuppressed, float InFadeOutDuration);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayDryFire();
    void Server_PlayDryFire_Implementation();
    bool Server_PlayDryFire_Validate();

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayDryFire();
    void Multi_PlayDryFire_Implementation();
    bool Multi_PlayDryFire_Validate();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayReload(bool bIsPistolClass);
    void Server_PlayReload_Implementation(bool bIsPistolClass);
    bool Server_PlayReload_Validate(bool bIsPistolClass);

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayReload(bool bIsPistolClass);
    void Multi_PlayReload_Implementation(bool bIsPistolClass);
    bool Multi_PlayReload_Validate(bool bIsPistolClass);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayReloadBulletDrop();
    void Server_PlayReloadBulletDrop_Implementation();
    bool Server_PlayReloadBulletDrop_Validate();

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayReloadBulletDrop();
    void Multi_PlayReloadBulletDrop_Implementation();
    bool Multi_PlayReloadBulletDrop_Validate();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlaySwitchFireMode();
    void Server_PlaySwitchFireMode_Implementation();
    bool Server_PlaySwitchFireMode_Validate();

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlaySwitchFireMode();
    void Multi_PlaySwitchFireMode_Implementation();
    bool Multi_PlaySwitchFireMode_Validate();

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_PlayEquipUnequipSuppressor();
    void Server_PlayEquipUnequipSuppressor_Implementation();
    bool Server_PlayEquipUnequipSuppressor_Validate();

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayEquipUnequipSuppressor();
    void Multi_PlayEquipUnequipSuppressor_Implementation();
    bool Multi_PlayEquipUnequipSuppressor_Validate();
};