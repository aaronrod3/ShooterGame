// Source/ShooterGame/Items/Weapon/Projectile.cpp

#include "Projectile.h"
#include "Components/BoxComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Framework/GameMode/ShooterGameGameMode.h"
#include "ShooterGame/Components/HitZoneComponent.h"
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
    // ── Cosmetic hit reaction — runs on all machines, unchanged ──
    AShooterGameCharacter* HitCharacter = Cast<AShooterGameCharacter>(OtherActor);
    if (HitCharacter)
    {
        HitCharacter->MulticastHit();
    }

    // ── Damage — server only, unchanged gate ──
    if (HasAuthority() && OtherActor && OtherActor != GetOwner())
    {
    	
    	// TEMP
    	// DIAGNOSTIC — remove after bone investigation
    	UE_LOG(LogTemp, Warning,
			TEXT("[DIAG] OtherActor: %s | OtherComp: %s | BoneName: %s | CompClass: %s"),
			*OtherActor->GetName(),
			OtherComp ? *OtherComp->GetName() : TEXT("NULL"),
			*Hit.BoneName.ToString(),
			OtherComp ? *OtherComp->GetClass()->GetName() : TEXT("NULL"));
    	
    	
        // ── Friendly fire check — unchanged ──
        AShooterGameCharacter* InstigatorCharacter = Cast<AShooterGameCharacter>(GetOwner());

        if (HitCharacter && InstigatorCharacter && HitCharacter != InstigatorCharacter)
        {
            AShooterGameGameMode* GameMode =
                GetWorld()->GetAuthGameMode<AShooterGameGameMode>();

            if (GameMode && !GameMode->IsFriendlyFireEnabled())
            {
                const bool bSameTeam = false; // TEMP — see original note
                if (bSameTeam)
                {
                    UE_LOG(LogTemp, Log,
                        TEXT("AProjectile::OnHit — Friendly fire blocked (%s -> %s)"),
                        *InstigatorCharacter->GetName(),
                        *OtherActor->GetName());

                    Destroy();
                    return;
                }
            }
        }

        // ── Damage resolution ──
        // CHANGED: Replaced raw bone-name string check and AmmoData multiplier
        // with HitZoneComponent lookup. This routes all actors — zombies and
        // players — through the same zone-aware damage path and eliminates the
        // parallel headshot detection that was duplicating zone logic.
        //
        // Hit.BoneName is populated here because OnComponentHit on a BoxComponent
        // colliding with a skeletal mesh physics asset body fills BoneName
        // natively — no extra trace needed.

        float AppliedDamage = FallbackDamage;

        if (AmmoDataAsset)
        {
            UHitZoneComponent* HitZoneComp =
                OtherActor->FindComponentByClass<UHitZoneComponent>();

            const FName BoneName = Hit.BoneName;
        	
            const float ZoneMultiplier = HitZoneComp
                ? HitZoneComp->GetDamageMultiplier(BoneName)
                : 1.f;

            AppliedDamage = AmmoDataAsset->BaseDamage * ZoneMultiplier;
        	
            const EHitZone HitZone = HitZoneComp
                ? HitZoneComp->GetHitZone(BoneName)
                : EHitZone::EHZ_Default;

            UE_LOG(LogTemp, Log,
                TEXT("[Projectile::OnHit] Hit: %s | Bone: %s | Zone: %d | Base: %.2f | Mult: %.2f | Final: %.2f"),
                *OtherActor->GetName(),
                *BoneName.ToString(),
                static_cast<int32>(HitZone),
                AmmoDataAsset->BaseDamage,
                ZoneMultiplier,
                AppliedDamage);
        }

        AController* InstigatorController = nullptr;
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
        {
            InstigatorController = OwnerPawn->GetController();
        }

        // ApplyPointDamage passes Hit (which contains BoneName) as
        // FPointDamageEvent into AZombieCharacter::TakeDamage (Piece 3).
        // TakeDamage reads FPointDamageEvent::HitInfo.BoneName to route
        // through HandleZombieHitZoneDamage — this is the correct connection.
        // Unchanged call site, the damage value is now zone-scaled.
        UGameplayStatics::ApplyPointDamage(
            OtherActor,
            AppliedDamage,
            GetActorForwardVector(),
            Hit,
            InstigatorController,
            this,
            UDamageType::StaticClass()
        );
    }

    // ── Surface impact audio — unchanged ──
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