// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CaseEject.generated.h"

UCLASS()
class SHOOTERGAME_API ACaseEject : public AActor
{
	GENERATED_BODY()

public:
	ACaseEject();
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	// In public section of header
	float GetSpawnTime() const { return SpawnTime; }
	

protected:
	virtual void BeginPlay() override;
	
	
	
	
private:

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* CasingMesh;
	
	
	UPROPERTY(EditAnywhere, Category = "Ejection")
	float EjectRightForce = 300.f;

	UPROPERTY(EditAnywhere, Category = "Ejection")
	float EjectUpForce = 150.f;

	UPROPERTY(EditAnywhere, Category = "Ejection")
	float EjectForwardForce = -50.f;   // slight backward kick

	// Variation ranges — randomness applied on top of base forces
	UPROPERTY(EditAnywhere, Category = "Ejection")
	float EjectVariation = 80.f;
	
	// Random spin on eject
	UPROPERTY(EditAnywhere, Category = "Ejection")
	float AngularImpulseStrength = 500.f;
	
	
	
	UPROPERTY(EditAnywhere)
	class USoundCue* ShellSound;
	
	// Hard cap before destroy regardless of physics state
	UPROPERTY(EditAnywhere)
	float MaxLifetime = 20.f;
	
	// How long casing persists after coming to rest
	UPROPERTY(EditAnywhere)
	float LifetimeAfterRest = 5.f;

	// Velocity threshold to consider casing "at rest"
	UPROPERTY(EditAnywhere)
	float RestVelocityThreshold = 1.f;

	// Max casings allowed in world before oldest is destroyed
	UPROPERTY(EditAnywhere)
	int32 MaxCasingCount = 20;

	// Interval in seconds between velocity checks
	UPROPERTY(EditAnywhere)
	float RestCheckInterval = 0.1f;

	bool bAtRest = false;
	bool bHasPlayedSound = false;

	float SpawnTime = 0.f;
	float TimeSinceLastCheck = 0.f;

	void CheckIfAtRest();
	void EnforceCasingLimit();
	
};





















