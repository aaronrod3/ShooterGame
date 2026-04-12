// Source/ShooterGame/Interaction/AmmoPickup.cpp

#include "AmmoPickup.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/TextBlock.h"

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
	
	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();

	// Only bind overlap on the server — CombatComponent pickup is authority-only
	if (HasAuthority())
	{
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AAmmoPickup::OnSphereOverlap);
		PickupSphere->OnComponentEndOverlap.AddDynamic(this, &AAmmoPickup::OnSphereEndOverlap);
	}
	
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
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
	if (ShooterCharacter)
	{
		ShooterCharacter->SetOverlappingAmmoPickup(this);
	}
}

void AAmmoPickup::OnSphereEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		ShooterCharacter->SetOverlappingAmmoPickup(nullptr);
	}
}


void AAmmoPickup::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);

		if (bShowWidget && PickupWidget->GetUserWidgetObject())
		{
			UTextBlock* TextBlock = Cast<UTextBlock>(
				PickupWidget->GetUserWidgetObject()->GetWidgetFromName(
					TEXT("TextBlock_PickupMessage")));  // ← same TextBlock name as weapon widget
			if (TextBlock)
			{
				TextBlock->SetText(FText::FromString(PickupMessage));
			}
		}
	}
}