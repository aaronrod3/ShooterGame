#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundCue.h"
#include "ImpactAudioComponent.generated.h"

struct FVector_NetQuantize;

UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UImpactAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UImpactAudioComponent();

	UFUNCTION(BlueprintCallable, Category = "ImpactAudio")
	void PlayImpactAtLocation_ForMultiplayer(const FVector& ImpactLocation, EPhysicalSurface SurfaceType);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "ImpactAudio|Cues")
	USoundCue* CueImpactMetal = nullptr;

	UPROPERTY(EditAnywhere, Category = "ImpactAudio|Cues")
	USoundCue* CueImpactConcrete = nullptr;

	UPROPERTY(EditAnywhere, Category = "ImpactAudio|Cues")
	USoundCue* CueImpactWood = nullptr;

	UPROPERTY(EditAnywhere, Category = "ImpactAudio|Cues")
	USoundCue* CueImpactDirt = nullptr;

	UPROPERTY(EditAnywhere, Category = "ImpactAudio|Cues")
	USoundCue* CueImpactDefault = nullptr;

	UPROPERTY(EditAnywhere, Category = "ImpactAudio|Cues")
	TArray<USoundCue*> CuesImpactFlesh;

	void Internal_PlayImpactAtLocation(const FVector& ImpactLocation, EPhysicalSurface SurfaceType);
	USoundCue* GetImpactCueForSurface(EPhysicalSurface SurfaceType) const;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_PlayImpactAtLocation(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType);
	void Server_PlayImpactAtLocation_Implementation(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType);
	bool Server_PlayImpactAtLocation_Validate(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType);

	UFUNCTION(NetMulticast, Reliable, WithValidation)
	void Multi_PlayImpactAtLocation(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType);
	void Multi_PlayImpactAtLocation_Implementation(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType);
	bool Multi_PlayImpactAtLocation_Validate(const FVector_NetQuantize& ImpactLocation, EPhysicalSurface SurfaceType);
};