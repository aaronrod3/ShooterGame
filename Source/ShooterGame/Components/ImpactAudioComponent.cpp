#include "ImpactAudioComponent.h"
#include "ShooterGame/Components/ShooterSoundFunctionLibrary.h"

UImpactAudioComponent::UImpactAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UImpactAudioComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UImpactAudioComponent::PlayImpactAtLocation_ForMultiplayer(const FVector& ImpactLocation, EPhysicalSurface SurfaceType)
{
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		Server_PlayImpactAtLocation(ImpactLocation, SurfaceType);
	}
	else if (GetOwner() && GetOwner()->HasAuthority())
	{
		Multi_PlayImpactAtLocation(ImpactLocation, SurfaceType);
	}
}

void UImpactAudioComponent::Internal_PlayImpactAtLocation(const FVector& ImpactLocation, EPhysicalSurface SurfaceType)
{
	USoundCue* CueToPlay = GetImpactCueForSurface(SurfaceType);
	if (!CueToPlay) return;

	UShooterSoundFunctionLibrary::PlaySoundAtLocation(this, CueToPlay, ImpactLocation);
}

USoundCue* UImpactAudioComponent::GetImpactCueForSurface(EPhysicalSurface SurfaceType) const
{
	switch (SurfaceType)
	{
	case SurfaceType1:
		return CueImpactMetal;

	case SurfaceType2:
		return CueImpactConcrete;

	case SurfaceType3:
		return CueImpactWood;

	case SurfaceType4:
		return CueImpactDirt;

	case SurfaceType5:
		if (CuesImpactFlesh.Num() > 0)
		{
			const int32 Index = FMath::RandRange(0, CuesImpactFlesh.Num() - 1);
			return CuesImpactFlesh[Index];
		}
		return CueImpactDefault;

	default:
		return CueImpactDefault;
	}
}

void UImpactAudioComponent::Server_PlayImpactAtLocation_Implementation(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType)
{
	Multi_PlayImpactAtLocation(ImpactLocation, SurfaceType);
}

bool UImpactAudioComponent::Server_PlayImpactAtLocation_Validate(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType)
{
	return true;
}

void UImpactAudioComponent::Multi_PlayImpactAtLocation_Implementation(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType)
{
	Internal_PlayImpactAtLocation(ImpactLocation, SurfaceType);
}

bool UImpactAudioComponent::Multi_PlayImpactAtLocation_Validate(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType)
{
	return true;
}