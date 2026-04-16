// Copyright SOUNDFX STUDIO © 2023

#include "WeaponSFXComponent_I.h"
#include "Sound/SoundCue.h"


// Sets default values for this component's properties
UWeaponSFXComponent_I::UWeaponSFXComponent_I()
{
}


// Called when the game starts
void UWeaponSFXComponent_I::BeginPlay()
{
	Super::BeginPlay();

	InitialiseAllSounds();
}


// Called every frame
void UWeaponSFXComponent_I::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}


// Initialise All Sounds
void UWeaponSFXComponent_I::InitialiseAllSounds()
{
	acs_Loop = CreateACs(cues_Loop);
	acs_Loop_Suppressor = CreateACs(cues_Loop_Suppressor);
}


// Play OneShot
void UWeaponSFXComponent_I::Play_OneShot()
{
	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_OneShot, GetOwner()->GetActorLocation());
	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_Stop, GetOwner()->GetActorLocation());
	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_Tail, GetOwner()->GetActorLocation());
}

// Play OneShot and Loop Firing Sounds
void UWeaponSFXComponent_I::Play_OneShotLoop(float crossfadeOneShotLoopTime)
{
	float offsetLoopTime = 0.f;
	if (offsetsLoopTime.Num() > 0)
		offsetLoopTime = offsetsLoopTime[FMath::RandRange(0, offsetsLoopTime.Num() - 1)];

	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_OneShot, GetOwner()->GetActorLocation());
	USoundFunctionLibrary_I::FadeInSounds(this, acs_Loop, crossfadeOneShotLoopTime, EAudioFaderCurve::Linear, offsetLoopTime - offsetOneShotTime);
	bIsLoopPlaying = true;
}

// Stop Loop Firing
void UWeaponSFXComponent_I::Stop_Loop(float fadeOutDuration)
{
	if (bIsLoopPlaying)
	{
		USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_Stop, GetOwner()->GetActorLocation());
		USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_Tail, GetOwner()->GetActorLocation());
		USoundFunctionLibrary_I::FadeOutSounds(this, acs_Loop, fadeOutDuration);
		bIsLoopPlaying = false;
	}
}

// Play Suppressor One Shot Sounds
void UWeaponSFXComponent_I::Play_OneShot_Suppressor()
{
	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_OneShot_Suppressor, GetOwner()->GetActorLocation());
	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_Stop_Suppressor, GetOwner()->GetActorLocation());
}

// Play Suppressor OneShot and Loop Firing Sounds
void UWeaponSFXComponent_I::Play_OneShotLoop_Suppressor(float crossfadeOneShotLoopTime)
{
	float offsetLoopTime = 0.f;
	if (offsetsLoopTime.Num() > 0)
		offsetLoopTime = offsetsLoopTime[FMath::RandRange(0, offsetsLoopTime.Num() - 1)];

	USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_OneShot_Suppressor, GetOwner()->GetActorLocation());
	USoundFunctionLibrary_I::FadeInSounds(this, acs_Loop_Suppressor, crossfadeOneShotLoopTime, EAudioFaderCurve::Linear, offsetLoopTime - offsetOneShotTime);
	bIsLoopPlaying = true;
}

// Stop Suppressor Loop Firing
void UWeaponSFXComponent_I::Stop_Loop_Suppressor(float fadeOutDuration)
{
	if (bIsLoopPlaying)
	{
		USoundFunctionLibrary_I::PlaySoundsAtLocation(this, cues_Stop_Suppressor, GetOwner()->GetActorLocation());
		USoundFunctionLibrary_I::FadeOutSounds(this, acs_Loop_Suppressor, fadeOutDuration);
		bIsLoopPlaying = false;
	}
}

// Play Switch Fire Mode Sound
void UWeaponSFXComponent_I::Play_SwitchFireMode()
{
	USoundFunctionLibrary_I::PlayAttachedShortDistanceSounds(this, cues_SwitchFireMode, GetOwner()->GetRootComponent());
}

// Play Equip/Unequip Suppressor Sound
void UWeaponSFXComponent_I::Play_EquipUnequipSuppressor()
{
	USoundFunctionLibrary_I::PlayAttachedShortDistanceSounds(this, cues_EquipUnequipSuppressor, GetOwner()->GetRootComponent());
}


// Create Audio Component
UAudioComponent* UWeaponSFXComponent_I::CreateAC(USoundCue * soundCue)
{
	if (soundCue)
	{
		UAudioComponent* ac = NewObject<UAudioComponent>(this, *soundCue->GetName());
		USoundFunctionLibrary_I::CreateAudioComponent(soundCue, GetOwner(), ac);
		return ac;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty SoundCue"));
		return nullptr;
	}
}

// Create Audio Components
TArray<UAudioComponent*> UWeaponSFXComponent_I::CreateACs(TArray<USoundCue*> soundCues)
{
	TArray<UAudioComponent*> acs;
	for (USoundCue* soundCue : soundCues)
	{
		acs.Add(CreateAC(soundCue));
	}
	return acs;
}

// FOR MULTIPLAYER
// 
void UWeaponSFXComponent_I::Play_OneShot_ForMultiplayer()
{
	if (GetOwner() == nullptr)
		return;

	Play_OneShot();

	if (!GetOwner()->HasAuthority())
		Server_Play_OneShot();
}

void UWeaponSFXComponent_I::Play_OneShotLoop_ForMultiplayer(float crossfadeOneShotLoopTime)
{
	if (GetOwner() == nullptr)
		return;

	Play_OneShotLoop(crossfadeOneShotLoopTime);

	if (!GetOwner()->HasAuthority())
		Server_Play_OneShotLoop(crossfadeOneShotLoopTime);
}

void UWeaponSFXComponent_I::Stop_Loop_ForMultiplayer(float fadeOutDuration)
{
	if (GetOwner() == nullptr)
		return;

	Stop_Loop(fadeOutDuration);

	if (!GetOwner()->HasAuthority())
		Server_Stop_Loop(fadeOutDuration);
}

void UWeaponSFXComponent_I::Play_OneShot_Suppressor_ForMultiplayer()
{
	if (GetOwner() == nullptr)
		return;

	Play_OneShot_Suppressor();

	if (!GetOwner()->HasAuthority())
		Server_Play_OneShot_Suppressor();
}

void UWeaponSFXComponent_I::Play_OneShotLoop_Suppressor_ForMultiplayer(float crossfadeOneShotLoopTime)
{
	if (GetOwner() == nullptr)
		return;

	Play_OneShotLoop_Suppressor(crossfadeOneShotLoopTime);

	if (!GetOwner()->HasAuthority())
		Server_Play_OneShotLoop_Suppressor(crossfadeOneShotLoopTime);
}

void UWeaponSFXComponent_I::Stop_Loop_Suppressor_ForMultiplayer(float fadeOutDuration)
{
	if (GetOwner() == nullptr)
		return;

	Stop_Loop_Suppressor(fadeOutDuration);

	if (!GetOwner()->HasAuthority())
		Server_Stop_Loop_Suppressor(fadeOutDuration);
}

void UWeaponSFXComponent_I::Play_SwitchFireMode_ForMultiplayer()
{
	if (GetOwner() == nullptr)
		return;

	Play_SwitchFireMode();

	if (!GetOwner()->HasAuthority())
		Server_Play_SwitchFireMode();
}

void UWeaponSFXComponent_I::Play_EquipUnequipSuppressor_ForMultiplayer()
{
	if (GetOwner() == nullptr)
		return;

	Play_EquipUnequipSuppressor();

	if (!GetOwner()->HasAuthority())
		Server_Play_EquipUnequipSuppressor();
}
// 
// Play One Shot Sounds
void UWeaponSFXComponent_I::Server_Play_OneShot_Implementation()
{
	Multi_Play_OneShot();
}
bool UWeaponSFXComponent_I::Server_Play_OneShot_Validate() { return true; }

void UWeaponSFXComponent_I::Multi_Play_OneShot_Implementation()
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_OneShot();
	}
}
bool UWeaponSFXComponent_I::Multi_Play_OneShot_Validate() { return true; }

void UWeaponSFXComponent_I::Server_Play_OneShotLoop_Implementation(float crossfadeOneShotLoopTime)
{
	Multi_Play_OneShotLoop(crossfadeOneShotLoopTime);
}
bool UWeaponSFXComponent_I::Server_Play_OneShotLoop_Validate(float crossfadeOneShotLoopTime) { return true; }

void UWeaponSFXComponent_I::Multi_Play_OneShotLoop_Implementation(float crossfadeOneShotLoopTime)
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_OneShotLoop(crossfadeOneShotLoopTime);
	}
}
bool UWeaponSFXComponent_I::Multi_Play_OneShotLoop_Validate(float crossfadeOneShotLoopTime) { return true; }

void UWeaponSFXComponent_I::Server_Stop_Loop_Implementation(float fadeOutDuration)
{
	Multi_Stop_Loop(fadeOutDuration);
}
bool UWeaponSFXComponent_I::Server_Stop_Loop_Validate(float fadeOutDuration) { return true; }

void UWeaponSFXComponent_I::Multi_Stop_Loop_Implementation(float fadeOutDuration)
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Stop_Loop(fadeOutDuration);
	}
}
bool UWeaponSFXComponent_I::Multi_Stop_Loop_Validate(float fadeOutDuration) {	return true; }

// Suppressed Functions
// 
void UWeaponSFXComponent_I::Server_Play_OneShot_Suppressor_Implementation()
{
	Multi_Play_OneShot_Suppressor();
}
bool UWeaponSFXComponent_I::Server_Play_OneShot_Suppressor_Validate() { return true; }

void UWeaponSFXComponent_I::Multi_Play_OneShot_Suppressor_Implementation()
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_OneShot_Suppressor();
	}
}
bool UWeaponSFXComponent_I::Multi_Play_OneShot_Suppressor_Validate() { return true; }

void UWeaponSFXComponent_I::Server_Play_OneShotLoop_Suppressor_Implementation(float crossfadeOneShotLoopTime)
{
	Multi_Play_OneShotLoop_Suppressor(crossfadeOneShotLoopTime);
}
bool UWeaponSFXComponent_I::Server_Play_OneShotLoop_Suppressor_Validate(float crossfadeOneShotLoopTime) {	return true; }

void UWeaponSFXComponent_I::Multi_Play_OneShotLoop_Suppressor_Implementation(float crossfadeOneShotLoopTime)
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_OneShotLoop_Suppressor(crossfadeOneShotLoopTime);
	}
}
bool UWeaponSFXComponent_I::Multi_Play_OneShotLoop_Suppressor_Validate(float crossfadeOneShotLoopTime) { return true; }

void UWeaponSFXComponent_I::Server_Stop_Loop_Suppressor_Implementation(float fadeOutDuration)
{
	Multi_Stop_Loop_Suppressor(fadeOutDuration);
}
bool UWeaponSFXComponent_I::Server_Stop_Loop_Suppressor_Validate(float fadeOutDuration) { return true; }

void UWeaponSFXComponent_I::Multi_Stop_Loop_Suppressor_Implementation(float fadeOutDuration)
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Stop_Loop_Suppressor(fadeOutDuration);
	}
}
bool UWeaponSFXComponent_I::Multi_Stop_Loop_Suppressor_Validate(float fadeOutDuration) { return true; }

void UWeaponSFXComponent_I::Server_Play_SwitchFireMode_Implementation()
{
	Multi_Play_SwitchFireMode();
}
bool UWeaponSFXComponent_I::Server_Play_SwitchFireMode_Validate() { return true; }

void UWeaponSFXComponent_I::Multi_Play_SwitchFireMode_Implementation()
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_SwitchFireMode();
	}
}
bool UWeaponSFXComponent_I::Multi_Play_SwitchFireMode_Validate() { return true; }

void UWeaponSFXComponent_I::Server_Play_EquipUnequipSuppressor_Implementation()
{
	Multi_Play_EquipUnequipSuppressor();
}
bool UWeaponSFXComponent_I::Server_Play_EquipUnequipSuppressor_Validate() { return true; }

void UWeaponSFXComponent_I::Multi_Play_EquipUnequipSuppressor_Implementation()
{
	if (GetOwner()->GetLocalRole() < ROLE_AutonomousProxy)
	{
		Play_EquipUnequipSuppressor();
	}
}
bool UWeaponSFXComponent_I::Multi_Play_EquipUnequipSuppressor_Validate() { return true; }