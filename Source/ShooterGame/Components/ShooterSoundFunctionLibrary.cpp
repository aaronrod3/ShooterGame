// Source/ShooterGame/Components/ShooterSoundFunctionLibrary.cpp

#include "ShooterSoundFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"

UAudioComponent* UShooterSoundFunctionLibrary::PlayAttachedShortDistance(
    USoundCue* SoundCue,
    USceneComponent* AttachToComponent,
    float VolumeMultiplier,
    float PitchMultiplier)
{
    if (!SoundCue || !AttachToComponent) return nullptr;

    return UGameplayStatics::SpawnSoundAttached(
        SoundCue,
        AttachToComponent,
        NAME_None,
        FVector::ZeroVector,
        EAttachLocation::SnapToTarget,
        /*bStopWhenAttachedToDestroyed=*/ true,
        VolumeMultiplier,
        PitchMultiplier
    );
}

UAudioComponent* UShooterSoundFunctionLibrary::PlaySoundAtLocation(
    UObject* WorldContextObject,
    USoundCue* SoundCue,
    FVector Location,
    float VolumeMultiplier,
    float PitchMultiplier)
{
    if (!SoundCue || !WorldContextObject) return nullptr;

    return UGameplayStatics::SpawnSoundAtLocation(
        WorldContextObject,
        SoundCue,
        Location,
        FRotator::ZeroRotator,
        VolumeMultiplier,
        PitchMultiplier
    );
}

UAudioComponent* UShooterSoundFunctionLibrary::Play2D(
    UObject* WorldContextObject,
    USoundCue* SoundCue,
    float VolumeMultiplier,
    float PitchMultiplier)
{
    if (!SoundCue || !WorldContextObject) return nullptr;

    return UGameplayStatics::SpawnSound2D(
        WorldContextObject,
        SoundCue,
        VolumeMultiplier,
        PitchMultiplier
    );
}

void UShooterSoundFunctionLibrary::FadeIn(
    UAudioComponent* AudioComp,
    float FadeInDuration,
    float TargetVolume,
    float StartTime)
{
    if (!AudioComp) return;
    AudioComp->FadeIn(FadeInDuration, TargetVolume, StartTime);
}

void UShooterSoundFunctionLibrary::FadeOut(
    UAudioComponent* AudioComp,
    float FadeOutDuration,
    float TargetVolume)
{
    if (!AudioComp) return;
    AudioComp->FadeOut(FadeOutDuration, TargetVolume);
}

void UShooterSoundFunctionLibrary::UpdateSound(
    UAudioComponent* AudioComp,
    USoundCue* NewSoundCue)
{
    if (!AudioComp || !NewSoundCue) return;
    AudioComp->SetSound(NewSoundCue);
    AudioComp->Play();
}

float UShooterSoundFunctionLibrary::GetSoundWaveDuration(USoundWave* SoundWave)
{
    if (!SoundWave) return 0.f;
    return SoundWave->GetDuration();
}