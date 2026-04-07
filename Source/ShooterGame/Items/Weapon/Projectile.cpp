// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Framework/GameMode/ShooterGameGameMode.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Projectile Movement Component"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;

	if (HasAuthority())
	{
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}

}


void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	
	if (TraceParticles){
		TraceParticlesComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TraceParticles,
		CollisionBox,
		FName(),
		GetActorLocation(),
		GetActorRotation(),
		EAttachLocation::KeepWorldPosition,
		true);
	}
	
	
}


void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AProjectile::OnHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	// Cosmetic hit reaction — unchanged
	AShooterGameCharacter* HitCharacter = Cast<AShooterGameCharacter>(OtherActor);
	if (HitCharacter)
	{
		HitCharacter->MulticastHit();
	}

	// Damage — server only
	if (HasAuthority() && OtherActor && OtherActor != GetOwner())
	{
		// -----------------------------------------------------------------------
		// Friendly fire check
		// -----------------------------------------------------------------------
		AShooterGameCharacter* InstigatorCharacter = Cast<AShooterGameCharacter>(GetOwner());

		if (HitCharacter && InstigatorCharacter && HitCharacter != InstigatorCharacter)
		{
			AShooterGameGameMode* GameMode =
				GetWorld()->GetAuthGameMode<AShooterGameGameMode>();

			if (GameMode && !GameMode->IsFriendlyFireEnabled())
			{
				// -------------------------------------------------------
				// CURRENT (PvE) — all AShooterGameCharacter are friendly
				// Any player hitting any other player is blocked when FF off
				// -------------------------------------------------------
				const bool bSameTeam = true; // all humans = same team in PvE

				// -------------------------------------------------------
				// FUTURE (PvP) — swap the line above with this:
				// const bool bSameTeam =
				//     HitCharacter->GetTeam() == InstigatorCharacter->GetTeam();
				// -------------------------------------------------------

				if (bSameTeam)
				{
					UE_LOG(LogTemp, Log,
						TEXT("AProjectile::OnHit — Friendly fire blocked (%s → %s)"),
						*InstigatorCharacter->GetName(),
						*HitCharacter->GetName());

					Destroy();
					return;
				}
				
			}
		}
		// -----------------------------------------------------------------------

		// Headshot check
		const bool bHeadShot =
			Hit.BoneName.ToString().ToLower().Contains(TEXT("head"));
		const float AppliedDamage =
			bHeadShot ? Damage * HeadShotMultiplier : Damage;

		AController* InstigatorController = nullptr;
		if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
		{
			InstigatorController = OwnerPawn->GetController();
		}

		UGameplayStatics::ApplyPointDamage(
			OtherActor,
			AppliedDamage,
			GetActorForwardVector(),
			Hit,
			InstigatorController,
			this,
			UDamageType::StaticClass()
		);

		UE_LOG(LogTemp, Log,
			TEXT("AProjectile::OnHit — Applied %.1f damage to %s%s"),
			AppliedDamage,
			*OtherActor->GetName(),
			bHeadShot ? TEXT(" [HEADSHOT]") : TEXT(""));
	}

	Destroy();
}


void AProjectile::Destroyed()
{
	Super::Destroyed();

	if (ImpactParticles)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactParticles,
			GetActorLocation(),
			GetActorRotation()
		);
	}
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}


void AProjectile::InitProjectile(float InDamage, float InHeadShotMultiplier)
{
	Damage             = InDamage;
	HeadShotMultiplier = InHeadShotMultiplier;
}