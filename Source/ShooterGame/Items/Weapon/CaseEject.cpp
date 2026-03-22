// Fill out your copyright notice in the Description page of Project Settings.


#include "CaseEject.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "EngineUtils.h"


// Sets default values
ACaseEject::ACaseEject()
{
	PrimaryActorTick.bCanEverTick = true;
	
	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Casing Mesh"));
	SetRootComponent(CasingMesh);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetSimulatePhysics(true);
	CasingMesh->SetEnableGravity(true);
	CasingMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = 5.f;
	
	// Casings are purely cosmetic, no need to replicate
	SetReplicates(false);
}


void ACaseEject::BeginPlay()
{
	Super::BeginPlay();
	
	CasingMesh->OnComponentHit.AddDynamic(this, &ACaseEject::OnHit);
	CasingMesh->AddImpulse(GetActorForwardVector() * ShellEjectionImpulse);
	
	// Record spawn time for accurate oldest-first sorting in the casing limit
	SpawnTime = GetWorld()->GetTimeSeconds();

	// Hard cap safety net — destroys casing if it never comes to rest
	SetLifeSpan(MaxLifetime);

	// Enforce pool limit on spawn to handle rapid fire
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
	if (ShellSound && !bHasPlayedSound)
	{
		bHasPlayedSound = true;
		UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
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
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACaseEject::StaticClass(), CasingsInWorld);

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


