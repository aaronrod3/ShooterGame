// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Projectile.h"


AProjectileWeapon::AProjectileWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AProjectileWeapon::BeginPlay()
{
	Super::BeginPlay();
	
}

void AProjectileWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if (!HasAuthority()) return;
	
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector MuzzleFlashLocation = SocketTransform.GetLocation();
		FVector FlatTarget = HitTarget;
		FlatTarget.Z = MuzzleFlashLocation.Z;
		FVector ToTarget = FlatTarget - MuzzleFlashLocation;
		FRotator TargetRotation = ToTarget.Rotation();
		TargetRotation.Pitch = 0.f;

		if (ProjectileClass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if (World)
			{
				AProjectile* SpawnedProjectile = World->SpawnActor<AProjectile>(
					ProjectileClass,
					MuzzleFlashLocation,
					TargetRotation,
					SpawnParams
				);

				if (SpawnedProjectile)
				{
					SpawnedProjectile->InitProjectile(GetDamage(), GetHeadShotMultiplier());
				}
			}
		}
	}
}