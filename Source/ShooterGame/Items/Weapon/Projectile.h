// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class AShooterGameCharacter;
class AController;


UCLASS()
class SHOOTERGAME_API AProjectile : public AActor
{
	GENERATED_BODY()

public:
	AProjectile();
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;
	
	// Called by AProjectileWeapon immediately after spawning to pass weapon damage values
	void InitProjectile(float InDamage, float InHeadShotMultiplier);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
private:
	
	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;
	
	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;
	
	
	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TraceParticles;
	UPROPERTY(EditAnywhere)
	class UNiagaraComponent* TraceParticlesComponent;
	
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactParticles;
	
	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;
	
	
	// Damage fallback — used only if the weapon's AmmoData asset is not set.
	// In normal gameplay AProjectileWeapon passes damage from AWeapon::GetDamage()
	// into the projectile at spawn time via InitProjectile().
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Damage")
	float FallbackDamage = 20.f;

	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Damage")
	float FallbackHeadShotMultiplier = 2.f;

	// Set by AProjectileWeapon::Fire() at spawn — carries weapon's actual damage values
	float Damage = 20.f;
	float HeadShotMultiplier = 2.f;
	
};
