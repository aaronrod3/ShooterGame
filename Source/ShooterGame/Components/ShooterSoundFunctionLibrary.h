// Source/ShooterGame/Components/ShooterSoundFunctionLibrary.h
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sound/SoundCue.h"
#include "Components/AudioComponent.h"
#include "ShooterSoundFunctionLibrary.generated.h"

/**
 * UShooterSoundFunctionLibrary
 *
 * Playback utility library — thin wrappers around UAudioComponent and
 * UGameplayStatics for consistent sound playback across all weapon audio
 * components in ShooterGame.
 *
 * Renamed from USoundFunctionLibraryS1 (SoundFX Studio 2023 plugin).
 * All SUPPRESSEDWEAPONSIAPI macros replaced with SHOOTERGAME_API.
 * _I variant discarded — only _S1 logic is used.
 */
UCLASS()
class SHOOTERGAME_API UShooterSoundFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Play a sound cue attached to a component (short-distance, positional).
     * Uses SpawnSoundAttached so the sound follows the component's location.
     */
    UFUNCTION(BlueprintCallable, Category = "ShooterAudio")
    static UAudioComponent* PlayAttachedShortDistance(
        USoundCue* SoundCue,
        USceneComponent* AttachToComponent,
        float VolumeMultiplier = 1.f,
        float PitchMultiplier  = 1.f
    );

    /**
     * Play a sound cue at a world location (non-attached, one-shot).
     */
    UFUNCTION(BlueprintCallable, Category = "ShooterAudio")
    static UAudioComponent* PlaySoundAtLocation(
        UObject* WorldContextObject,
        USoundCue* SoundCue,
        FVector Location,
        float VolumeMultiplier = 1.f,
        float PitchMultiplier  = 1.f
    );

    /**
     * Play a 2D (non-positional) sound cue — UI, notifications, etc.
     */
    UFUNCTION(BlueprintCallable, Category = "ShooterAudio")
    static UAudioComponent* Play2D(
        UObject* WorldContextObject,
        USoundCue* SoundCue,
        float VolumeMultiplier = 1.f,
        float PitchMultiplier  = 1.f
    );

    /**
     * Fade in an already-created UAudioComponent over FadeInDuration seconds.
     */
    UFUNCTION(BlueprintCallable, Category = "ShooterAudio")
    static void FadeIn(
        UAudioComponent* AudioComp,
        float FadeInDuration = 0.1f,
        float TargetVolume   = 1.f,
        float StartTime      = 0.f
    );

    /**
     * Fade out an already-created UAudioComponent over FadeOutDuration seconds.
     */
    UFUNCTION(BlueprintCallable, Category = "ShooterAudio")
    static void FadeOut(
        UAudioComponent* AudioComp,
        float FadeOutDuration = 0.2f,
        float TargetVolume    = 0.f
    );

    /**
     * Update a looping AudioComponent's sound and restart it.
     * Used when switching between suppressed/unsuppressed loop cues mid-fire.
     */
    UFUNCTION(BlueprintCallable, Category = "ShooterAudio")
    static void UpdateSound(
        UAudioComponent* AudioComp,
        USoundCue* NewSoundCue
    );

    /**
     * Returns the duration of a SoundWave in seconds.
     * Returns 0.f if SoundWave is null.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ShooterAudio")
    static float GetSoundWaveDuration(USoundWave* SoundWave);
};