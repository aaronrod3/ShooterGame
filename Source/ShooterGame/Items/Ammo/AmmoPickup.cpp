// Source/ShooterGame/Interaction/AmmoPickup.cpp

#include "AmmoPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Components/CombatComponent.h"

AAmmoPickup::AAmmoPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Mesh — visual representation of the ammo pickup in the world
	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Sphere — overlap trigger for pickup detection
	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(RootComponent);
	PickupSphere->SetSphereRadius(64.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();

	// Only bind overlap on the server — CombatComponent pickup is authority-only
	if (HasAuthority())
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AAmmoPickup::OnSphereOverlap);
	}
}

void AAmmoPickup::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(OtherActor);
	if (!ShooterCharacter) return;

	// Validate the magazine has rounds before granting —
	// prevents empty pickups from wasting inventory slots
	if (GrantedMagazine.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AAmmoPickup::OnSphereOverlap — GrantedMagazine is empty, pickup ignored"));
		return;
	}

	UCombatComponent* Combat = ShooterCharacter->GetCombat();
	if (!Combat) return;

	Combat->PickupMagazine(GrantedMagazine);

	UE_LOG(LogTemp, Log,
		TEXT("AAmmoPickup — %s picked up %d rounds of ammo"),
		*ShooterCharacter->GetName(),
		GrantedMagazine.CurrentRounds);

	// Consume the pickup — destroy on server, replication removes it on clients
	Destroy();
}