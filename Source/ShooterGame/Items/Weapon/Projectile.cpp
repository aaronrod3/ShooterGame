// Source/ShooterGame/Items/Weapon/Projectile.cpp

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
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
	
	ImpactAudioComp = CreateDefaultSubobject<UImpactAudioComponent>(TEXT("ImpactAudioComp"));
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (TraceParticles)
	{
		TraceParticlesComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TraceParticles,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition,
			true
		);
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
	// Cosmetic hit reaction — runs on all machines
	AShooterGameCharacter* HitCharacter = Cast<AShooterGameCharacter>(OtherActor);
	if (HitCharacter)
	{
		HitCharacter->MulticastHit();
	}

	// Damage — server only
	if (HasAuthority() && OtherActor && OtherActor != GetOwner())
	{
		// -------------------------------------------------------------------
		// Friendly fire check
		// NOTE: bSameTeam is set to false here temporarily for damage testing.
		// When team/PvP logic is implemented, replace with:
		//     const bool bSameTeam = HitCharacter->GetTeam() == InstigatorCharacter->GetTeam();
		// -------------------------------------------------------------------
		AShooterGameCharacter* InstigatorCharacter = Cast<AShooterGameCharacter>(GetOwner());

		if (HitCharacter && InstigatorCharacter && HitCharacter != InstigatorCharacter)
		{
			AShooterGameGameMode* GameMode =
				GetWorld()->GetAuthGameMode<AShooterGameGameMode>();

			if (GameMode && !GameMode->IsFriendlyFireEnabled())
			{
				const bool bSameTeam = false; // TEMP: disabled for ammo/damage testing — see note above
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
		// -------------------------------------------------------------------

		// -------------------------------------------------------------------
		// Damage resolution — reads from AmmoData asset
		// Body shot: BaseDamage
		// Head shot: BaseDamage * HeadShotMultiplier (bone name contains "head")
		// No asset assigned: falls back to FallbackDamage
		// -------------------------------------------------------------------
		float AppliedDamage = FallbackDamage;

		if (AmmoDataAsset)
		{
			const bool bHeadShot = Hit.BoneName.ToString().ToLower().Contains(TEXT("head"));
			AppliedDamage = bHeadShot
				? AmmoDataAsset->BaseDamage * AmmoDataAsset->HeadShotMultiplier
				: AmmoDataAsset->BaseDamage;
		}

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
			TEXT("AProjectile::OnHit — Hit: %s | Bone: %s | Damage: %.1f"),
			*OtherActor->GetName(),
			*Hit.BoneName.ToString(),
			AppliedDamage);
	}
	
	const EPhysicalSurface SurfaceType =
		UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

	if (ImpactAudioComp)
	{
		ImpactAudioComp->PlayImpactAtLocation_ForMultiplayer(Hit.ImpactPoint, SurfaceType);
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
}

void AProjectile::InitProjectile(UAmmoData* InAmmoData)
{
	AmmoDataAsset = InAmmoData;
}