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
 * Handles all weapon gunshot audio for a single AWeapon actor.
 * Mirrors the layout of the purchased SoundFX Studio plugin so cue arrays
 * can be filled in the editor exactly the same way.
 *
 * Suppressor routing: queries AWeapon::HasSuppressor() at fire time —
 * no manual switching needed. Just fill both cue sets in the weapon BP.
 *
 * Multiplayer: all play/stop calls go through Server → NetMulticast RPCs
 * so every connected client hears the correct sound.
 *
 * Usage:
 *   - Add to AWeapon constructor via CreateDefaultSubobject
 *   - Fill cue arrays in each weapon's Blueprint defaults
 *   - Call PlayGunshot_ForMultiplayer() from AWeapon::Fire()
 *   - Call StopLoop_ForMultiplayer() when firing stops (full-auto only)
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

    // Single-shot fire sounds (semi-auto, first shot of burst)
    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_OneShot;

    // Loop layer — plays after the first shot for full-auto sustained fire
    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_Loop;

    // Plays when full-auto loop stops (mechanical tail / reverb tail)
    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Unsuppressed")
    TArray<USoundCue*> Cues_Stop;

    // Acoustic tail — plays after the stop cue fades out
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

    // -----------------------------------------------------------------------
    // Timing Offsets (match plugin defaults)
    // -----------------------------------------------------------------------

    // Seconds before the loop layer kicks in after the first shot
    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Offsets")
    TArray<float> OffsetLoopTimes;

    // Seconds before the one-shot layer plays (aligns transient to loop)
    UPROPERTY(EditAnywhere, Category = "WeaponSounds|Offsets")
    float OffsetOneShotTime = 0.04f;

    // -----------------------------------------------------------------------
    // Public API — call these from AWeapon
    // -----------------------------------------------------------------------

    /**
     * Main fire call for single-shot and semi-auto weapons.
     * Automatically picks suppressed or unsuppressed cues based on
     * AWeapon::HasSuppressor(). Routes through Server → Multicast RPC.
     */
    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayGunshot_ForMultiplayer();

    /**
     * Main fire call for full-auto weapons — plays one-shot then starts loop.
     * @param CrossfadeTime seconds between one-shot and loop starting
     */
    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void PlayGunshotLoop_ForMultiplayer(float CrossfadeTime = 0.05f);

    /**
     * Stops the full-auto loop and plays stop + tail cues.
     * Call when the player releases fire on a full-auto weapon.
     * @param FadeOutDuration seconds to fade out the loop layer
     */
    UFUNCTION(BlueprintCallable, Category = "WeaponAudio")
    void StopLoop_ForMultiplayer(float FadeOutDuration = 0.2f);

protected:
    virtual void BeginPlay() override;

private:
    // -----------------------------------------------------------------------
    // Internal Audio Component Management
    // -----------------------------------------------------------------------

    // Creates and attaches a single UAudioComponent for a cue
    UAudioComponent* CreateAudioComp(USoundCue* SoundCue);

    // Creates UAudioComponents for a full cue array
    TArray<UAudioComponent*> CreateAudioComps(const TArray<USoundCue*>& SoundCues);

    // Called in BeginPlay — pre-creates all audio components so there's
    // no allocation cost at fire time
    void InitialiseAllSounds();

    // Pre-created audio components — avoids runtime allocation on fire
    TArray<UAudioComponent*> ACs_Start;
    TArray<UAudioComponent*> ACs_Loop;
    TArray<UAudioComponent*> ACs_Start_Suppressed;
    TArray<UAudioComponent*> ACs_Loop_Suppressed;

    bool bIsLoopPlaying = false;

    // -----------------------------------------------------------------------
    // Internal local play functions (called on all machines via Multicast)
    // -----------------------------------------------------------------------
    void Internal_PlayOneShot(bool bSuppressed);
    void Internal_PlayOneShotLoop(bool bSuppressed, float CrossfadeTime);
    void Internal_StopLoop(bool bSuppressed, float FadeOutDuration);

    // Queries the owning AWeapon for current suppressor state
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
    void Server_PlayGunshotLoop(bool bSuppressed, float CrossfadeTime);
    void Server_PlayGunshotLoop_Implementation(bool bSuppressed, float CrossfadeTime);
    bool Server_PlayGunshotLoop_Validate(bool bSuppressed, float CrossfadeTime);

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_PlayGunshotLoop(bool bSuppressed, float CrossfadeTime);
    void Multi_PlayGunshotLoop_Implementation(bool bSuppressed, float CrossfadeTime);
    bool Multi_PlayGunshotLoop_Validate(bool bSuppressed, float CrossfadeTime);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_StopLoop(bool bSuppressed, float FadeOutDuration);
    void Server_StopLoop_Implementation(bool bSuppressed, float FadeOutDuration);
    bool Server_StopLoop_Validate(bool bSuppressed, float FadeOutDuration);

    UFUNCTION(NetMulticast, Reliable, WithValidation)
    void Multi_StopLoop(bool bSuppressed, float FadeOutDuration);
    void Multi_StopLoop_Implementation(bool bSuppressed, float FadeOutDuration);
    bool Multi_StopLoop_Validate(bool bSuppressed, float FadeOutDuration);
};