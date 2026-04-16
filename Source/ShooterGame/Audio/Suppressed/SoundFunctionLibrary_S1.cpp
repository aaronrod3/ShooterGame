// Copyright SOUNDFX STUDIO © 2023


#include "SoundFunctionLibrary_S1.h"
#include "Sound/SoundCue.h"
#include <Sound/SoundNodeWavePlayer.h>
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"


// Create Audio Component
void USoundFunctionLibrary_S1::CreateAudioComponent(USoundCue* soundCue, AActor* owner, UAudioComponent* out)
{
	if (!soundCue)
		return;

	out->SetWorldLocation(owner->GetActorLocation());
	out->SetWorldRotation(owner->GetActorRotation());
	out->AttachToComponent(owner->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform, NAME_None);
	out->bAutoActivate = false;

	FString cueName = soundCue->GetName();

	out->SetSound(soundCue);
	out->RegisterComponent();
}


// Play Sound at Location
void USoundFunctionLibrary_S1::PlaySoundAtLocation(const UObject* worldContextObject, USoundCue* cue, FVector location, float volumeMultiplier, float pitchMultiplier, FRotator rotation, float startDelay)
{
	if (volumeMultiplier <= 0.f) return;

	const UObject* safeContext = GetSafeContext(worldContextObject);

	if (cue && safeContext && safeContext->GetWorld())
	{
		UGameplayStatics::PlaySoundAtLocation(safeContext, cue, location, rotation, volumeMultiplier, pitchMultiplier);
	}
}

// Play Sounds at Location
void USoundFunctionLibrary_S1::PlaySoundsAtLocation(const UObject* worldContextObject, TArray<USoundCue*> cues, FVector location, float volumeMultiplier, float pitchMultiplier, FRotator rotation, float startDelay)
{
	if (volumeMultiplier <= 0.f) return;
	
	for (USoundCue* cue : cues)
	{
		if (cue)
			USoundFunctionLibrary_S1::PlaySoundAtLocation(worldContextObject, cue, location, volumeMultiplier, pitchMultiplier, rotation, startDelay);
	}
}

// Play Sound attached to SceneComponent and destroyed after playing complete. The distance delay won't be calculated, so avoid using it with the sounds with a big attenuation radiuses
void USoundFunctionLibrary_S1::PlayAttachedShortDistanceSound(const UObject* worldContextObject, USoundCue* cue, USceneComponent* attachTo, float volumeMultiplier, float pitchMultiplier, FRotator rotation)
{
	if (volumeMultiplier <= 0.f) return;

	const UObject* safeContext = GetSafeContext(worldContextObject);

	if (cue && safeContext && safeContext->GetWorld())
	{
		UGameplayStatics::SpawnSoundAttached(cue, attachTo, NAME_None, FVector(ForceInit), rotation, EAttachLocation::KeepRelativeOffset, true, volumeMultiplier, pitchMultiplier);
	}
}

// Play Sounds attached to SceneComponent and destroyed after playing complete. The distance delay won't be calculated, so avoid using it with the sounds with a big attenuation radiuses
void USoundFunctionLibrary_S1::PlayAttachedShortDistanceSounds(const UObject* worldContextObject, TArray<USoundCue*> cues, USceneComponent* attachTo, float volumeMultiplier, float pitchMultiplier, FRotator rotation)
{
	if (volumeMultiplier <= 0.f) return;
	
	for (USoundCue* cue : cues)
	{
		if (cue)
			USoundFunctionLibrary_S1::PlayAttachedShortDistanceSound(worldContextObject, cue, attachTo, volumeMultiplier, pitchMultiplier, rotation);
	}
}

void USoundFunctionLibrary_S1::Play2DSound(const UObject* worldContextObject, USoundCue* cue, float volumeMultiplier, float pitchMultiplier)
{
	if (volumeMultiplier <= 0.f) return;

	const UObject* safeContext = GetSafeContext(worldContextObject);

	if (cue && safeContext && safeContext->GetWorld())
	{
		UGameplayStatics::SpawnSound2D(worldContextObject, cue, volumeMultiplier, pitchMultiplier);
	}
}

void USoundFunctionLibrary_S1::Play2DSounds(const UObject* worldContextObject, TArray<USoundCue*> cues, float volumeMultiplier, float pitchMultiplier)
{
	if (volumeMultiplier <= 0.f) return;
	
	for (USoundCue* cue : cues)
	{
		if (cue)
			USoundFunctionLibrary_S1::Play2DSound(worldContextObject, cue, volumeMultiplier, pitchMultiplier);
	}
}


void USoundFunctionLibrary_S1::UpdateSound(UAudioComponent* ac, float vol, float pitch)
{
	if (ac)
	{
		ac->SetVolumeMultiplier(vol);
		ac->SetPitchMultiplier(pitch);
	}
}

void USoundFunctionLibrary_S1::UpdateSounds(TArray<UAudioComponent*> acs, float vol, float pitch)
{
	for (UAudioComponent* ac : acs)
	{
		UpdateSound(ac, vol, pitch);
	}
}


// Link Volume and Pitch params from one AudioComponent to Another
void USoundFunctionLibrary_S1::LinkParams(UAudioComponent* ac, TArray<UAudioComponent*> acs, FString acLinkName)
{
	for (UAudioComponent* audioComponent : acs)
	{
		FString acName = *(audioComponent->GetName());

		if (acName == acLinkName)
		{
			ac->SetVolumeMultiplier(audioComponent->VolumeMultiplier);
			ac->SetPitchMultiplier(audioComponent->PitchMultiplier);
			break;
		}
	}
}


// Link Volume and Pitch params from one AudioComponent to Another
void USoundFunctionLibrary_S1::LinkParams(UAudioComponent* ac, UAudioComponent* acLink)
{
	ac->SetVolumeMultiplier(acLink->VolumeMultiplier);
	ac->SetPitchMultiplier(acLink->PitchMultiplier);
}


// FadeOut Sounds
void USoundFunctionLibrary_S1::FadeOutSounds(const UObject* worldContextObject, TArray<UAudioComponent*> acs, float fadeOutDuration, const EAudioFaderCurve fadeCurve)
{
	for (UAudioComponent* ac : acs)
	{
		USoundFunctionLibrary_S1::FadeOutSound(worldContextObject, ac, fadeOutDuration, fadeCurve);
	}
}


// FadeOut Sound
void USoundFunctionLibrary_S1::FadeOutSound(const UObject* worldContextObject, UAudioComponent* ac, float fadeOutDuration, const EAudioFaderCurve fadeCurve)
{
	if (ac)
	{
		ac->FadeOut(fadeOutDuration, 0.f, fadeCurve);
	}
}

// FadeIn Sounds
void USoundFunctionLibrary_S1::FadeInSounds(const UObject* worldContextObject, TArray<UAudioComponent*> acs, float fadeInDuration, const EAudioFaderCurve fadeCurve, float startTime)
{
	for (UAudioComponent* ac : acs)
	{
		USoundFunctionLibrary_S1::FadeInSound(worldContextObject, ac, fadeInDuration, fadeCurve, startTime);
	}
}


// FadeIn Sound
void USoundFunctionLibrary_S1::FadeInSound(const UObject* worldContextObject, UAudioComponent* ac, float fadeInDuration, const EAudioFaderCurve fadeCurve, float startTime)
{
	if (ac)
	{
		if (!ac->IsPlaying() || ac->bIsFadingOut)
		{
			ac->FadeIn(fadeInDuration, 1.f, startTime, fadeCurve);
		}
	}
}


float USoundFunctionLibrary_S1::GetSoundWaveDuration(USoundCue* cue, int number)
{
	TArray<USoundNodeWavePlayer*> wavePlayers;
	if (cue)
		cue->RecursiveFindNode<USoundNodeWavePlayer>(cue->FirstNode, wavePlayers);
	float duration = 0.f;
	if (wavePlayers.Num() > number)
		duration = wavePlayers[number]->GetSoundWave()->Duration;

	return duration;
}


const UObject* USoundFunctionLibrary_S1::GetSafeContext(const UObject* Object)
{
	const UObject* context = Object ? (Object->GetWorld() ? Object->GetWorld() : Object) : nullptr;
	check(context != nullptr);

	return context;
}