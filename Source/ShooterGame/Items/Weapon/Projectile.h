// Source/ShooterGame/Items/Weapon/Projectile.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterGame/Items/Ammo/AmmoData.h"
#include "ShooterGame/Components/ImpactAudioComponent.h"
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

	/**
	 * Called by AProjectileWeapon immediately after spawning.
	 * Passes the weapon's UAmmoData asset so this projectile resolves
	 * body/head damage from the asset rather than hardcoded floats.
	 */
	void InitProjectile(UAmmoData* InAmmoData);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnHit(
		UPrimitiveComponent* HitComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit
	);

private:

	UPROPERTY(EditAnywhere)
	class UBoxComponent* CollisionBox;

	UPROPERTY(VisibleAnywhere)
	class UProjectileMovementComponent* ProjectileMovementComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile|Audio", meta=( AllowPrivateAccess = "true"))
	UImpactAudioComponent* ImpactAudioComp;

	UPROPERTY(EditAnywhere)
	class UNiagaraSystem* TraceParticles;

	UPROPERTY(VisibleAnywhere)
	class UNiagaraComponent* TraceParticlesComponent;

	UPROPERTY(EditAnywhere)
	UNiagaraSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	class USoundCue* ImpactSound;

	/**
	 * Ammo data asset assigned at spawn by AProjectileWeapon::Fire().
	 * Provides BaseDamage and HeadShotMultiplier for this shot.
	 * If null (e.g. weapon has no AmmoData assigned), FallbackDamage is used.
	 */
	UPROPERTY()
	UAmmoData* AmmoDataAsset = nullptr;

	// Safety fallback — only used if weapon has no AmmoData asset assigned
	UPROPERTY(EditDefaultsOnly, Category = "Projectile|Damage")
	float FallbackDamage = 20.f;
};