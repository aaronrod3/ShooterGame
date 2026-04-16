// Copyright SOUNDFX STUDIO © 2023

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/GameplayStatics.h"
#include "SoundFunctionLibrary_I.h"
#include "Net/UnrealNetwork.h"
#include "WeaponSFXComponent_I.generated.h"


UCLASS(Blueprintable, BlueprintType)
class WEAPONS_SOUNDS_I_API UWeaponSFXComponent_I : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponSFXComponent_I();

public:	

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// SOUND CUES
	////////////////////////////////////////////////////////////////////
	// One Shot sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_OneShot;
	// Loop sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_Loop;
	// Stop and Tail sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_Stop;
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_Tail;

	// Suppressor One Shot sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_OneShot_Suppressor;
	// Loop sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_Loop_Suppressor;
	// Stop and Tail sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_Stop_Suppressor;

	// Switch Fire Mode sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_SwitchFireMode;

	// Equip/Unequip Suppressor sounds
	UPROPERTY(EditAnywhere, Category = WeaponSounds)
		TArray<USoundCue*> cues_EquipUnequipSuppressor;



	////////////////////////////////////////////////////////////////////
	// FUNCTIONS
	////////////////////////////////////////////////////////////////////
	// Play One Shot Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Play_OneShot();

	// Play OneShot and Loop Firing Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Play_OneShotLoop(float crossfadeOneShotLoopTime = 0.05f);
	
	// Stop Loop Firing Sounds and Play End and Tail Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Stop_Loop(float fadeOutDuration = 0.2f);

	// Play One Shot Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Play_OneShot_Suppressor();

	// Play OneShot and Loop Firing Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Play_OneShotLoop_Suppressor(float crossfadeOneShotLoopTime = 0.05f);

	// Stop Loop Firing Sounds and Play End and Tail Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Stop_Loop_Suppressor(float fadeOutDuration = 0.2f);

	// Switch Fire Mode Sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Play_SwitchFireMode();

	// Equip/Unequip Suppressor sounds
	UFUNCTION(BlueprintCallable, Category = SoundsFunctions)
		void Play_EquipUnequipSuppressor();


	// SET Weapon PARAMETERS
	////////////////////////////////////////////////////////////////////
	// Offsets for starting loop
	UPROPERTY(EditAnywhere, Category = SoundOffsets)
		TArray<float> offsetsLoopTime;
	// Primary offset for Oneshot
	UPROPERTY(EditAnywhere, Category = SoundOffsets)
		float offsetOneShotTime = 0.04f;

protected:

	// Called when the game starts
	virtual void BeginPlay() override;

	UAudioComponent* CreateAC(USoundCue* soundCue);

	// Create Audio Components
	TArray<UAudioComponent*> CreateACs(TArray<USoundCue*> soundCues);

	// Sounds Audio Components
	TArray<UAudioComponent*> acs_Start;
	TArray<UAudioComponent*> acs_Loop;

	TArray<UAudioComponent*> acs_Start_Suppressor;
	TArray<UAudioComponent*> acs_Loop_Suppressor;

	bool bIsLoopPlaying = false;

	// Initialise All Sounds
	void InitialiseAllSounds();

	// FOR MULTIPLAYER
public:
	// Play One Shot Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Play_OneShot_ForMultiplayer();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Play_OneShot();
	void Server_Play_OneShot_Implementation();
	bool Server_Play_OneShot_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Play_OneShot();
	void Multi_Play_OneShot_Implementation();
	bool Multi_Play_OneShot_Validate();

	// Play OneShot and Loop Firing Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Play_OneShotLoop_ForMultiplayer(float crossfadeOneShotLoopTime = 0.05f);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Play_OneShotLoop(float crossfadeOneShotLoopTime);
	void Server_Play_OneShotLoop_Implementation(float crossfadeOneShotLoopTime);
	bool Server_Play_OneShotLoop_Validate(float crossfadeOneShotLoopTime);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Play_OneShotLoop(float crossfadeOneShotLoopTime);
	void Multi_Play_OneShotLoop_Implementation(float crossfadeOneShotLoopTime);
	bool Multi_Play_OneShotLoop_Validate(float crossfadeOneShotLoopTime);

	// Stop Loop Firing Sounds and Play End and Tail Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Stop_Loop_ForMultiplayer(float fadeOutDuration = 0.2f);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Stop_Loop(float fadeOutDuration);
	void Server_Stop_Loop_Implementation(float fadeOutDuration);
	bool Server_Stop_Loop_Validate(float fadeOutDuration);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Stop_Loop(float fadeOutDuration);
	void Multi_Stop_Loop_Implementation(float fadeOutDuration);
	bool Multi_Stop_Loop_Validate(float fadeOutDuration);

	// Suppressed Functions
		// Play One Shot Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Play_OneShot_Suppressor_ForMultiplayer();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Play_OneShot_Suppressor();
	void Server_Play_OneShot_Suppressor_Implementation();
	bool Server_Play_OneShot_Suppressor_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Play_OneShot_Suppressor();
	void Multi_Play_OneShot_Suppressor_Implementation();
	bool Multi_Play_OneShot_Suppressor_Validate();

	// Play OneShot and Loop Firing Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Play_OneShotLoop_Suppressor_ForMultiplayer(float crossfadeOneShotLoopTime = 0.05f);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Play_OneShotLoop_Suppressor(float crossfadeOneShotLoopTime);
	void Server_Play_OneShotLoop_Suppressor_Implementation(float crossfadeOneShotLoopTime);
	bool Server_Play_OneShotLoop_Suppressor_Validate(float crossfadeOneShotLoopTime);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Play_OneShotLoop_Suppressor(float crossfadeOneShotLoopTime);
	void Multi_Play_OneShotLoop_Suppressor_Implementation(float crossfadeOneShotLoopTime);
	bool Multi_Play_OneShotLoop_Suppressor_Validate(float crossfadeOneShotLoopTime);

	// Stop Loop Firing Sounds and Play End and Tail Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Stop_Loop_Suppressor_ForMultiplayer(float fadeOutDuration = 0.2f);

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Stop_Loop_Suppressor(float fadeOutDuration);
	void Server_Stop_Loop_Suppressor_Implementation(float fadeOutDuration);
	bool Server_Stop_Loop_Suppressor_Validate(float fadeOutDuration);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Stop_Loop_Suppressor(float fadeOutDuration);
	void Multi_Stop_Loop_Suppressor_Implementation(float fadeOutDuration);
	bool Multi_Stop_Loop_Suppressor_Validate(float fadeOutDuration);

	// Switch Fire Mode Sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Play_SwitchFireMode_ForMultiplayer();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Play_SwitchFireMode();
	void Server_Play_SwitchFireMode_Implementation();
	bool Server_Play_SwitchFireMode_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Play_SwitchFireMode();
	void Multi_Play_SwitchFireMode_Implementation();
	bool Multi_Play_SwitchFireMode_Validate();

	// Equip/Unequip Suppressor sounds for Multiplayer Game
	UFUNCTION(BlueprintCallable, Category = SoundsFunctionsForMultiplayer)
		void Play_EquipUnequipSuppressor_ForMultiplayer();

	UFUNCTION(Server, Reliable, WithValidation)
		void Server_Play_EquipUnequipSuppressor();
	void Server_Play_EquipUnequipSuppressor_Implementation();
	bool Server_Play_EquipUnequipSuppressor_Validate();

	UFUNCTION(NetMulticast, Reliable, WithValidation)
		void Multi_Play_EquipUnequipSuppressor();
	void Multi_Play_EquipUnequipSuppressor_Implementation();
	bool Multi_Play_EquipUnequipSuppressor_Validate();
};