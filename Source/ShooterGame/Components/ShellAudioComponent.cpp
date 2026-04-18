// Source/ShooterGame/Components/ShellAudioComponent.cpp

#include "ShellAudioComponent.h"
#include "ShooterSoundFunctionLibrary.h"
#include "PhysicsEngine/PhysicsSettings.h"

UShellAudioComponent::UShellAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UShellAudioComponent::BeginPlay()
{
	Super::BeginPlay();
}

USoundCue* UShellAudioComponent::GetCueForSurface(EPhysicalSurface SurfaceType) const
{
	// SurfaceType enum values are defined in Project Settings → Physics → Physical Surfaces.
	// Step 6 will define PM_Metal=1, PM_Concrete=2, PM_Wood=3, PM_Dirt=4.
	// Until then, all casings play Cue_Default.
	switch (SurfaceType)
	{
	case EPhysicalSurface::SurfaceType1: return Cue_Metal;     // PM_Metal
	case EPhysicalSurface::SurfaceType2: return Cue_Concrete;  // PM_Concrete
	case EPhysicalSurface::SurfaceType3: return Cue_Wood;      // PM_Wood
	case EPhysicalSurface::SurfaceType4: return Cue_Dirt;      // PM_Dirt
	default:                             return Cue_Default;
	}
}

void UShellAudioComponent::PlayImpactSound(FVector ImpactLocation, EPhysicalSurface SurfaceType)
{
	USoundCue* CueToPlay = GetCueForSurface(SurfaceType);
	if (!CueToPlay) CueToPlay = Cue_Default;
	if (!CueToPlay) return;

	UShooterSoundFunctionLibrary::PlaySoundAtLocation(this, CueToPlay, ImpactLocation);
}