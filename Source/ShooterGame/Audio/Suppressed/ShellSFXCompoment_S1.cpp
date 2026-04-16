// Copyright SOUNDFX STUDIO © 2023

#include "ShellSFXCompoment_S1.h"
#include "Sound/SoundCue.h"


// Sets default values for this component's properties
UShellSFXCompoment_S1::UShellSFXCompoment_S1()
{
}


// Called when the game starts
void UShellSFXCompoment_S1::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UShellSFXCompoment_S1::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


void UShellSFXCompoment_S1::Play_AmmunitionCasings(FVector location, float hitValue, EPhysicalSurface surfaceType)
{
	USoundFunctionLibrary_S1::PlaySoundAtLocation(this, CasingsSoundsSurface[surfaceType].Cue_AmmunitionCasings, location, hitValue);
}


// FOR MULTIPLAYER
void UShellSFXCompoment_S1::Play_AmmunitionCasings_ForMultiplayer(FVector location, float hitValue, EPhysicalSurface surfaceType)
{
	if (GetOwner() == nullptr)
		return;

	Play_AmmunitionCasings(location, hitValue, surfaceType);

	if (!GetOwner()->HasAuthority())
		Server_Play_AmmunitionCasings(location, hitValue, surfaceType);
}

void UShellSFXCompoment_S1::Server_Play_AmmunitionCasings_Implementation(FVector location, float hitValue, EPhysicalSurface surfaceType)
{
	Multi_Play_AmmunitionCasings(location, hitValue, surfaceType);
}
bool UShellSFXCompoment_S1::Server_Play_AmmunitionCasings_Validate(FVector location, float hitValue, EPhysicalSurface surfaceType) { return true; }

void UShellSFXCompoment_S1::Multi_Play_AmmunitionCasings_Implementation(FVector location, float hitValue, EPhysicalSurface surfaceType)
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_AmmunitionCasings(location, hitValue, surfaceType);
	}
}
bool UShellSFXCompoment_S1::Multi_Play_AmmunitionCasings_Validate(FVector location, float hitValue, EPhysicalSurface surfaceType) { return true; }