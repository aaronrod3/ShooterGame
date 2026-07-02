// Source/ShooterGame/Items/Weapon/Weapon.cpp

#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimationAsset.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Items/Weapon/CaseEject.h"
#include "ShooterGame/Items/Weapon/WeaponConfig.h"
#include "ShooterGame/Framework/ShooterGame.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootComponent);
	CollisionSphere->SetSphereRadius(96.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
	
	WeaponAudioComp = CreateDefaultSubobject<UWeaponAudioComponent>(TEXT("WeaponAudioComp"));
	AudioPerceptionComp = CreateDefaultSubobject<UAudioPerceptionComponent>(TEXT("AudioPerceptionComp"));
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereOverlap);
		CollisionSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}

	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	DecaySpread(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME(AWeapon, bRoundChambered);
	DOREPLIFETIME(AWeapon, ReplicatedMagState);
	DOREPLIFETIME(AWeapon, bHasSuppressor);
}


void AWeapon::InitFromConfig(UWeaponConfig* InConfig)
{
	if (!InConfig)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AWeapon::InitFromConfig — null config passed to %s"), *GetName());
		return;
	}

	WeaponConfig = InConfig;

	// -----------------------------------------------------------------------
	// Receiver mesh + AnimBP
	// -----------------------------------------------------------------------
	if (USkeletalMesh* ReceiverMesh = InConfig->MeshReceiver.LoadSynchronous())
	{
		WeaponMesh->SetSkeletalMesh(ReceiverMesh);
	}

	if (InConfig->ABP_Weapon && WeaponMesh)
	{
		WeaponMesh->SetAnimInstanceClass(InConfig->ABP_Weapon);
	}

	// -----------------------------------------------------------------------
	// Optional static mesh attachments
	// Each part only attaches if both the mesh and socket are configured.
	// -----------------------------------------------------------------------
	MeshComp_Handguard  = AttachStaticMeshFromConfig(InConfig->MeshHandguard,  InConfig->SocketHandguard);
	MeshComp_Scope      = AttachStaticMeshFromConfig(InConfig->MeshScope,      InConfig->SocketScope);
	MeshComp_SightFront = AttachStaticMeshFromConfig(InConfig->MeshSightFront, InConfig->SocketSightFront);
	MeshComp_SightRear  = AttachStaticMeshFromConfig(InConfig->MeshSightRear,  InConfig->SocketSightRear);
	MeshComp_Silencer   = AttachStaticMeshFromConfig(InConfig->MeshSilencer,   InConfig->SocketMuzzle);

	UE_LOG(LogTemp, Log,
		TEXT("AWeapon::InitFromConfig — '%s' assembled from config '%s'"),
		*GetName(), *InConfig->GetName());
}


UStaticMeshComponent* AWeapon::AttachStaticMeshFromConfig(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket)
{
	if (!WeaponMesh || Socket.IsNone())
	{
		return nullptr;
	}

	UStaticMesh* LoadedMesh = Mesh.LoadSynchronous();
	if (!LoadedMesh)
	{
		return nullptr;
	}

	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(this);
	if (!MeshComp)
	{
		return nullptr;
	}

	MeshComp->SetStaticMesh(LoadedMesh);
	MeshComp->SetupAttachment(WeaponMesh, Socket);
	MeshComp->RegisterComponent();
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetGenerateOverlapEvents(false);

	return MeshComp;
}




// -----------------------------------------------------------------------
// Overlap / Pickup Widget
// -----------------------------------------------------------------------

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		ShooterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AShooterGameCharacter* ShooterCharacter = Cast<AShooterGameCharacter>(OtherActor);
	if (ShooterCharacter)
	{
		ShooterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);

		// ── When showing, push PickupMessage directly into the TextBlock ──
		if (bShowWidget && PickupWidget->GetUserWidgetObject())
		{
			UTextBlock* TextBlock = Cast<UTextBlock>(
				PickupWidget->GetUserWidgetObject()->GetWidgetFromName(
					TEXT("TextBlock_PickupMessage")));  // ← must match your TextBlock's name exactly
			if (TextBlock)
			{
				TextBlock->SetText(FText::FromString(PickupMessage));
			}
		}
		// ────────────────────────────────────────────────────────────────
	}
}

// -----------------------------------------------------------------------
// Weapon State
// -----------------------------------------------------------------------

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;

	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	default:
		break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		break;
	default:
		break;
	}
}

// -----------------------------------------------------------------------
// Fire
// -----------------------------------------------------------------------

void AWeapon::Fire(const FVector& HitTarget)
{
	if (!CanFire())
	{
		return;
	}

	// Play fire animation
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	// Spawn case eject
	if (CasingClass)
	{
		const USkeletalMeshSocket* CaseSocket = WeaponMesh->GetSocketByName(FName("CaseEject"));
		if (CaseSocket)
		{
			FTransform SocketTransform = CaseSocket->GetSocketTransform(WeaponMesh);
			if (UWorld* World = GetWorld())
			{
				World->SpawnActor<ACaseEject>(
					CasingClass,
					SocketTransform.GetLocation(),
					SocketTransform.GetRotation().Rotator()
				);
			}
		}
	}

	AddSpreadOnFire();

	// -------------------------------------------------------------------
	// Audio — route based on fire mode
	// Semi/Burst: one-shot cue
	// Full-Auto:  one-shot + loop cue start
	// -------------------------------------------------------------------
	if (WeaponAudioComp)
	{
		if (CurrentFireMode == EFireMode::EFM_FullAuto)
		{
			WeaponAudioComp->PlayGunshotLoop_ForMultiplayer();
		}
		else
		{
			WeaponAudioComp->PlayGunshot_ForMultiplayer();
		}
	}
	
	
	
	// -------------------------------------------------------------------
	// AI hearing — only on real shots. Suppressed = shorter radius + lower loudness.
	// This is the ONLY place ReportNoiseEvent should fire for weapons.
	// -------------------------------------------------------------------
	if (AudioPerceptionComp)
	{
		const float PerceptionRadius = bHasSuppressor
			? AudioPerceptionComp->SuppressedNoiseRadius
			: -1.f; // -1 = use DefaultEmitRadius as-is

		const float PerceptionLoudness = bHasSuppressor
			? AudioPerceptionComp->SuppressedLoudness
			: -1.f; // -1 = use Loudness as-is

		AudioPerceptionComp->EmitSoundEvent(PerceptionRadius, PerceptionLoudness);
	}
}

void AWeapon::StopFire()
{
	if (!WeaponAudioComp) return;

	/*
	if (CurrentFireMode == EFireMode::EFM_FullAuto)
	{
		WeaponAudioComp->StopLoop_ForMultiplayer();
	}
	*/
	
	//TEMP
	if (CurrentFireMode == EFireMode::EFM_FullAuto)
	{
		UE_LOG(LogTemp, Warning, TEXT("AWeapon::StopFire — calling StopLoop_ForMultiplayer"));
		WeaponAudioComp->StopLoop_ForMultiplayer();
	}
}

// -----------------------------------------------------------------------
// Ammo / Feed
// -----------------------------------------------------------------------

bool AWeapon::CanFire() const
{
	// A chambered round is always available to fire
	if (bRoundChambered) return true;

	// Rounds in the inserted magazine
	if (InsertedMagazine.IsSet() && !InsertedMagazine.GetValue().IsEmpty()) return true;

	return false;
}

bool AWeapon::ConsumeRound()
{
	if (!CanFire()) return false;

	if (bRoundChambered)
	{
		bRoundChambered = false;

		// Auto-feed: advance the next round from the magazine into the chamber
		if (InsertedMagazine.IsSet() && !InsertedMagazine.GetValue().IsEmpty())
		{
			InsertedMagazine.GetValue().ConsumeRound();
			bRoundChambered = true;
		}

		SyncReplicatedLoadState();
		OnAmmoChanged.Broadcast(GetLoadedRounds(), GetMagCapacity());
		return true;
	}

	// No chamber support (cylinder, single-shot) — consume directly from magazine
	if (InsertedMagazine.IsSet() && InsertedMagazine.GetValue().ConsumeRound())
	{
		SyncReplicatedLoadState();
		OnAmmoChanged.Broadcast(GetLoadedRounds(), GetMagCapacity());
		return true;
	}

	return false;
}

void AWeapon::InsertMagazine(const FMagazine& NewMag)
{
	if (NewMag.AmmoType != SupportedAmmoType)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("AWeapon::InsertMagazine — Caliber mismatch. Weapon accepts %d, mag is %d"),
			(int32)SupportedAmmoType, (int32)NewMag.AmmoType);
		return;
	}

	InsertedMagazine = NewMag;
	SyncReplicatedLoadState();
}


FMagazine AWeapon::EjectMagazine()
{
	FMagazine Ejected;

	if (InsertedMagazine.IsSet())
	{
		Ejected = InsertedMagazine.GetValue();
		InsertedMagazine.Reset();
		SyncReplicatedLoadState();
	}

	return Ejected;
}

void AWeapon::ChamberRound()
{
	// Weapon doesn't support a chambered round (revolver, launcher)
	if (!bSupportsChamberedRound) return;

	// Already chambered
	if (bRoundChambered) return;

	// No magazine to chamber from
	if (!InsertedMagazine.IsSet()) return;

	if (InsertedMagazine.GetValue().ConsumeRound())
	{
		bRoundChambered = true;
		SyncReplicatedLoadState();
	}
}

void AWeapon::SpawnDroppedMagazine(float ImpulseForce, float RotationForce)
{
	UE_LOG(LogShooterGame, Warning, TEXT("AWeapon::SpawnDroppedMagazine — not yet implemented (stub called on %s)"), *GetName());
}

void AWeapon::EjectCasing(FRotator RotationOffset, float MinEjectForce, float MaxEjectForce, float RotationSpeed)
{
	UE_LOG(LogShooterGame, Warning, TEXT("AWeapon::EjectCasing — not yet implemented (stub called on %s)"), *GetName());
}

void AWeapon::SetMagazineVisibility(bool bVisible, bool bReserve)
{
	UE_LOG(LogShooterGame, Warning, TEXT("AWeapon::SetMagazineVisibility — not yet implemented (stub called on %s)"), *GetName());
}

void AWeapon::SyncReplicatedLoadState()
{
	if (InsertedMagazine.IsSet())
	{
		ReplicatedMagState = InsertedMagazine.GetValue();
	}
	else
	{
		ReplicatedMagState = FMagazine();
		ReplicatedMagState.Capacity = 0;
	}
}

void AWeapon::OnRep_LoadState()
{
	if (ReplicatedMagState.Capacity > 0)
	{
		InsertedMagazine = ReplicatedMagState;
	}
	else
	{
		InsertedMagazine.Reset();
	}
	
	OnAmmoChanged.Broadcast(GetLoadedRounds(), GetMagCapacity());
}

// -----------------------------------------------------------------------
// Spread
// -----------------------------------------------------------------------

void AWeapon::AddSpreadOnFire()
{
	CurrentSpread = FMath::Clamp(CurrentSpread + SpreadIncreasePerShot, 0.f, MaxSpread);
}

void AWeapon::DecaySpread(float DeltaTime)
{
	CurrentSpread = FMath::Max(0.f, CurrentSpread - SpreadDecayRate * DeltaTime);
}

// -----------------------------------------------------------------------
// Fire Mode
// -----------------------------------------------------------------------

void AWeapon::CycleFireMode()
{
	if (AllowedFireModes.Num() <= 1) return;

	int32 CurrentIndex = AllowedFireModes.IndexOfByKey(CurrentFireMode);
	int32 NextIndex = (CurrentIndex + 1) % AllowedFireModes.Num();
	CurrentFireMode = AllowedFireModes[NextIndex];
}

void AWeapon::ApplySpreadMultiplier(float Multiplier)
{
	// Clamp to max spread — don't let debuff exceed the weapon's ceiling
	CurrentSpread = FMath::Min(CurrentSpread * Multiplier, MaxSpread);
}


// -----------------------------------------------------------------------
// Suppressor Attachment
// -----------------------------------------------------------------------

// Source/ShooterGame/Items/Weapon/Weapon.cpp
// Place after AWeapon::ApplySpreadMultiplier()

void AWeapon::SetSuppressor(bool bEquipped)
{
	// Must only be called on the server — UCombatComponent enforces this.
	if (!HasAuthority()) return;

	if (bHasSuppressor == bEquipped) return; // No change, no broadcast

	bHasSuppressor = bEquipped;

	// Broadcast on server immediately (RepNotify only auto-fires on clients)
	OnRep_SuppressorState();
}

void AWeapon::OnRep_SuppressorState()
{
	// Fires on clients via replication, and manually on the server via SetSuppressor().
	// Notify any Blueprint listeners (e.g., weapon mesh visibility toggle,
	// equip/unequip sound). WeaponAudioComponent reads bHasSuppressor directly
	// at fire time, so no additional call is needed here for audio routing.
	OnSuppressorChanged.Broadcast(bHasSuppressor);

	UE_LOG(LogTemp, Log, TEXT("AWeapon::OnRep_SuppressorState — Suppressor: %s on %s"),
		bHasSuppressor ? TEXT("EQUIPPED") : TEXT("REMOVED"),
		*GetName());
}