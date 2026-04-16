// Copyright SOUNDFX STUDIO © 2023

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundAttenuation.h"
#include "TimerManager.h"
#include "Components/AudioComponent.h"
#include "SoundFunctionLibrary_S1.generated.h"


UCLASS(Blueprintable)
class SUPPRESSED_WEAPONS_I_API USoundFunctionLibrary_S1 : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	////////////////////////////////////////////////////////////////////
	// FUNCTIONS
	////////////////////////////////////////////////////////////////////
	// Create Audio Component
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
	static void CreateAudioComponent(USoundCue* soundCue, AActor* owner, UAudioComponent* out);

	//Play Sound At Location
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void PlaySoundAtLocation(const UObject* worldContextObject, USoundCue* cue, FVector location, float volumeMultiplier = 1.0f, float pitchMultiplier = 1.0f, FRotator rotation = FRotator::ZeroRotator, float startDelay = 0.f);

	//Play Sounds At Location
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void PlaySoundsAtLocation(const UObject* worldContextObject, TArray<USoundCue*> cues, FVector location, float volumeMultiplier = 1.0f, float pitchMultiplier = 1.0f, FRotator rotation = FRotator::ZeroRotator, float startDelay = 0.f);

	// Play Sound attached to SceneComponent and destroyed after playing complete. The distance delay won't be calculated, so avoid using it with the sounds with a big attenuation radiuses
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void PlayAttachedShortDistanceSound(const UObject* worldContextObject, USoundCue* cue, USceneComponent* attachTo, float volumeMultiplier = 1.0f, float pitchMultiplier = 1.0f, FRotator rotation = FRotator::ZeroRotator);

	// Play Sounds attached to SceneComponent and destroyed after playing complete. The distance delay won't be calculated, so avoid using it with the sounds with a big attenuation radiuses
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void PlayAttachedShortDistanceSounds(const UObject* worldContextObject, TArray<USoundCue*> cues, USceneComponent* attachTo, float volumeMultiplier = 1.0f, float pitchMultiplier = 1.0f, FRotator rotation = FRotator::ZeroRotator);

	// Play 2D Sound
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void Play2DSound(const UObject* worldContextObject, USoundCue* cue, float volumeMultiplier = 1.0f, float pitchMultiplier = 1.0f);

	// Play 2D Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void Play2DSounds(const UObject* worldContextObject, TArray<USoundCue*> cues, float volumeMultiplier = 1.0f, float pitchMultiplier = 1.0f);

	// Update Sound Params
	static void UpdateSound(UAudioComponent* ac, float vol, float pitch = 1.f);

	// Update Sounds Params
	static void UpdateSounds(TArray<UAudioComponent*> acs, float vol, float pitch = 1.f);

	// Link Volume and Pitch params from one AudioComponent to Another
	static void LinkParams(UAudioComponent* ac, TArray<UAudioComponent*> acs, FString acLinkName);
	static void LinkParams(UAudioComponent* ac, UAudioComponent* acLink);

	// FadeOut Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void FadeOutSounds(const UObject* worldContextObject, TArray<UAudioComponent*> acs, float fadeOutDuration, const EAudioFaderCurve fadeCurve = EAudioFaderCurve::Linear);
	
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void FadeOutSound(const UObject* worldContextObject, UAudioComponent* ac, float fadeOutDuration, const EAudioFaderCurve fadeCurve = EAudioFaderCurve::Linear);

	// FadeIn Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void FadeInSounds(const UObject* worldContextObject, TArray<UAudioComponent*> acs, float fadeInDuration, const EAudioFaderCurve fadeCurve = EAudioFaderCurve::Linear, float startTime = 0.0f);
	
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions, meta = (WorldContext = "worldContextObject"))
	static void FadeInSound(const UObject* worldContextObject, UAudioComponent* ac, float fadeInDuration, const EAudioFaderCurve fadeCurve = EAudioFaderCurve::Linear, float startTime = 0.0f);

	// Get Sound Wave Duration
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
	static float GetSoundWaveDuration(USoundCue* cue, int number = 0);

	static const UObject* GetSafeContext(const UObject* Object);
};
