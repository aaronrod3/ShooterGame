// Fill out your copyright notice in the Description page of Project Settings.


#include "CaseEject.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "EngineUtils.h"


// Sets default values
ACaseEject::ACaseEject()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing Mesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	
	
	// Casings are purely cosmetic, no need to replicate
	SetReplicates(false);
	
	ShellAudioComp = CreateDefaultSubobject<UShellAudioComponent>(TEXT("ShellAudioComp"));
}


void ACaseEject::BeginPlay()
{
	Super::BeginPlay();
	
	CasingMesh->OnComponentHit.AddDynamic(this, &ACaseEject::OnHit);
	
	// Zero out any inherited velocity from the spawning actor's movement
	CasingMesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	CasingMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	
	// Build directional impulse with per-axis variation
	FVector EjectDirection =
	GetActorRightVector()   * (EjectRightForce  + FMath::FRandRange(-EjectVariation, EjectVariation)) +
	GetActorUpVector()      * (EjectUpForce      + FMath::FRandRange(-EjectVariation, EjectVariation)) +
	GetActorForwardVector() * (EjectForwardForce + FMath::FRandRange(-EjectVariation, EjectVariation));

	CasingMesh->AddImpulse(EjectDirection);

	// Random angular spin so casing tumbles differently each time
	FVector RandomSpin = FVector(
		FMath::FRandRange(-AngularImpulseStrength, AngularImpulseStrength),
		FMath::FRandRange(-AngularImpulseStrength, AngularImpulseStrength),
		FMath::FRandRange(-AngularImpulseStrength, AngularImpulseStrength)
	);
	CasingMesh->AddAngularImpulseInDegrees(RandomSpin);
	
	SpawnTime = GetWorld()->GetTimeSeconds();
	SetLifeSpan(MaxLifetime);
	EnforceCasingLimit();
	
}


void ACaseEject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!bAtRest)
	{
		TimeSinceLastCheck += DeltaTime;

		if (TimeSinceLastCheck >= RestCheckInterval)
		{
			TimeSinceLastCheck = 0.f;
			CheckIfAtRest();
		}
	}
}

void ACaseEject::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// Only play sound on first impact to prevent bounce spam
	if (!bHasPlayedSound)
	{
		bHasPlayedSound = true;

		// Derive surface type from physical material at impact point
		const EPhysicalSurface SurfaceType =
			UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get());

		if (ShellAudioComp)
		{
			ShellAudioComp->PlayImpactSound(GetActorLocation(), SurfaceType);
		}
	}
}

void ACaseEject::CheckIfAtRest()
{
	if (!CasingMesh) return;

	FVector Velocity = CasingMesh->GetPhysicsLinearVelocity();

	if (Velocity.Size() <= RestVelocityThreshold)
	{
		bAtRest = true;

		// Disable physics — casing is static, no need to simulate further
		CasingMesh->SetSimulatePhysics(false);

		// Disable tick — nothing left to poll
		PrimaryActorTick.bCanEverTick = false;

		// Override the 20s hard cap with the shorter post-rest lifetime
		SetLifeSpan(LifetimeAfterRest);
	}
}


void ACaseEject::EnforceCasingLimit()
{
	TArray<AActor*> CasingsInWorld;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), StaticClass(), CasingsInWorld);

	if (CasingsInWorld.Num() > MaxCasingCount)
	{
		// Sort by SpawnTime ascending so oldest casings are destroyed first
		CasingsInWorld.Sort([](const AActor& A, const AActor& B)
		{
			const ACaseEject* CasingA = Cast<ACaseEject>(&A);
			const ACaseEject* CasingB = Cast<ACaseEject>(&B);
			if (CasingA && CasingB)
			{
				return CasingA->SpawnTime < CasingB->SpawnTime;
			}
			return false;
		});

		int32 NumToRemove = CasingsInWorld.Num() - MaxCasingCount;
		for (int32 i = 0; i < NumToRemove; i++)
		{
			if (CasingsInWorld[i] && CasingsInWorld[i] != this)
			{
				CasingsInWorld[i]->Destroy();
			}
		}
	}
}


