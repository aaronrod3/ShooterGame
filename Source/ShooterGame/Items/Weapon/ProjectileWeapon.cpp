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
	if (!InstigatorPawn) return;

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if (!MuzzleFlashSocket) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector MuzzleFlashLocation = SocketTransform.GetLocation();

	const FVector ToTarget = HitTarget - MuzzleFlashLocation;
	const FVector ShootDirection = ToTarget.GetSafeNormal();
	const FRotator TargetRotation = ShootDirection.Rotation();

	if (ProjectileClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		if (UWorld* World = GetWorld())
		{
			AProjectile* SpawnedProjectile = World->SpawnActor<AProjectile>(
				ProjectileClass,
				MuzzleFlashLocation,
				TargetRotation,
				SpawnParams
			);

			if (SpawnedProjectile)
			{
				SpawnedProjectile->InitProjectile(GetAmmoData());
			}
		}
	}
}