// Source/ShooterGame/Enemy/Character/ZombieCharacter.cpp

#include "ZombieCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "ZombieAnimInstance.h"
#include "ShooterGame/Components/HitZoneComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

AZombieCharacter::AZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    bAlwaysRelevant = true;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
    bUseControllerRotationYaw = false;

    HitZoneComponent = CreateDefaultSubobject<UHitZoneComponent>(TEXT("HitZoneComponent"));
}

void AZombieCharacter::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: BeginPlay — HasAuthority: %s — NetMode: %d"),
        HasAuthority() ? TEXT("YES") : TEXT("NO"),
        (int32)GetNetMode());

    if (!HasAuthority()) return;

    Health = ZombieConfig.MaxHealth;
    GetCharacterMovement()->MaxWalkSpeed = ZombieConfig.WalkSpeed;
    bCanSprint = FMath::FRand() < ZombieConfig.SprintChance;

    if (ZombieAIControllerClass)
    {
        AZombieAIController* ZombieController = GetWorld()->SpawnActor<AZombieAIController>(
            ZombieAIControllerClass,
            GetActorLocation(),
            GetActorRotation()
        );

        if (ZombieController)
        {
            ZombieController->Possess(this);
            UE_LOG(LogTemp, Warning, TEXT("ZombieCharacter: Possessed by %s"), *ZombieController->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ZombieCharacter: Failed to spawn controller"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("ZombieCharacter: ZombieAIControllerClass not set — assign BP_ZombieAIController in Class Defaults"));
    }
}

void AZombieCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
}

void AZombieCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AZombieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AZombieCharacter, Health);
    DOREPLIFETIME(AZombieCharacter, ZombieState);
    DOREPLIFETIME(AZombieCharacter, bCanSprint);
}

// ─────────────────────────────────────────────
// Damage
// ─────────────────────────────────────────────

float AZombieCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (!HasAuthority()) return 0.f;
    if (ZombieState == EZombieState::EZS_Dead) return 0.f;

    // Crawler instant-kill — already at 1 HP from ApplyCrawlerPhysics().
    // Any hit finishes it regardless of zone. Preserve this path exactly.
    if (ZombieState == EZombieState::EZS_Crawling)
    {
        Health = 0.f;
        HandleDeath(false);
        return DamageAmount;
    }

    // Point damage from a weapon carries a bone name — route through hit zone system.
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID) && HitZoneComponent)
    {
        const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);

        const FName    BoneName = PointDamage->HitInfo.BoneName;
        const EHitZone HitZone  = HitZoneComponent->GetHitZone(BoneName);

        HandleZombieHitZoneDamage(
            HitZone,
            BoneName,
            DamageAmount,
            PointDamage->HitInfo,
            EventInstigator,
            DamageCauser
        );

        // Return the incoming damage amount for engine bookkeeping.
        // Actual health deduction only happens inside HandleZombieHeadshot.
        // Body-zone hits are intentionally nonlethal — no health subtracted here.
        return DamageAmount;
    }

    // Fallback: non-point damage (melee from another system, explosions, fall damage, etc.)
    // Use the original flat HP depletion for anything that doesn't carry hit-zone context.
    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    Health = FMath::Clamp(Health - DamageAmount, 0.f, ZombieConfig.MaxHealth);

    if (Health <= 0.f)
    {
        HandleDeath(false);
    }

    return ActualDamage;
}


// ─────────────────────────────────────────────
// Hit Zone Routing
// ─────────────────────────────────────────────


void AZombieCharacter::HandleZombieHitZoneDamage(
    EHitZone HitZone,
    FName BoneName,
    float DamageAmount,
    const FHitResult& HitInfo,
    AController* EventInstigator,
    AActor* DamageCauser)
{
    switch (HitZone)
    {
    case EHitZone::EHZ_Head:
    case EHitZone::EHZ_Neck:
        HandleZombieHeadshot(HitZone, BoneName, DamageAmount, HitInfo, EventInstigator, DamageCauser);
        break;

    case EHitZone::EHZ_Torso:
        HandleZombieTorsoHit(DamageAmount, HitInfo);
        break;

    case EHitZone::EHZ_Arm:
        HandleZombieArmHit(BoneName, DamageAmount, HitInfo);
        break;

    case EHitZone::EHZ_Leg:
        HandleZombieLegHit(BoneName, DamageAmount, HitInfo);
        break;

    case EHitZone::EHZ_Default:
    default:
        // Bone hit but not mapped — treat as nonlethal torso.
        HandleZombieTorsoHit(DamageAmount, HitInfo);
        break;
    }
}

// ─────────────────────────────────────────────
// Hit Zone Handlers
// ─────────────────────────────────────────────

void AZombieCharacter::HandleZombieHeadshot(
    EHitZone /*HitZone*/,
    FName BoneName,
    float DamageAmount,
    const FHitResult& /*HitInfo*/,
    AController* /*EventInstigator*/,
    AActor* /*DamageCauser*/)
{
    if (ZombieState == EZombieState::EZS_Dead || bHeadDestroyed)
    {
        return;
    }

    // Check scaled damage against lethal threshold.
    // Ammo depth rule: weak pistol rounds may not clear the threshold on heavy zombie
    // variants if HeadshotKillDamageThreshold is raised in the BP subclass.
    if (DamageAmount < HeadshotKillDamageThreshold)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[HitZone] %s survived %s hit — %.2f damage below lethal threshold %.2f"),
            *GetName(), *BoneName.ToString(), DamageAmount, HeadshotKillDamageThreshold);
        return;
    }

    bHeadDestroyed = true;

    // Set health to zero before HandleDeath so any secondary health checks are clean.
    Health = 0.f;

    UE_LOG(LogTemp, Log,
        TEXT("[HitZone] %s — lethal headshot on %s for %.2f damage"),
        *GetName(), *BoneName.ToString(), DamageAmount);

    // Pass bWasHeadshot = true so HandleDeath skips the crawler roll entirely.
    // Headshots always produce clean instant death per Phase 1B design intent.
    HandleDeath(true);
}

void AZombieCharacter::HandleZombieTorsoHit(float DamageAmount, const FHitResult& HitInfo)
{
    if (ZombieState == EZombieState::EZS_Dead)
    {
        return;
    }
    
    UE_LOG(LogTemp, Log,
        TEXT("[HitZone] %s — torso hit for %.2f at bone %s"),
        *GetName(), DamageAmount, *HitInfo.BoneName.ToString());
}

void AZombieCharacter::HandleZombieArmHit(FName BoneName, float DamageAmount, const FHitResult& /*HitInfo*/)
{
    if (ZombieState == EZombieState::EZS_Dead)
    {
        return;
    }

    if (IsLeftArmBone(BoneName) && !bLeftArmDisabled)
    {
        LeftArmAccumulatedDamage += DamageAmount;

        if (LeftArmAccumulatedDamage >= ArmDisableDamageThreshold)
        {
            bLeftArmDisabled = true;
            UE_LOG(LogTemp, Log, TEXT("[HitZone] %s — left arm disabled"), *GetName());
        }

        return;
    }

    if (IsRightArmBone(BoneName) && !bRightArmDisabled)
    {
        RightArmAccumulatedDamage += DamageAmount;

        if (RightArmAccumulatedDamage >= ArmDisableDamageThreshold)
        {
            bRightArmDisabled = true;
            UE_LOG(LogTemp, Log, TEXT("[HitZone] %s — right arm disabled"), *GetName());
        }

        return;
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("[HitZone] %s — arm hit on unmapped side bone %s"),
        *GetName(), *BoneName.ToString());
}

void AZombieCharacter::HandleZombieLegHit(FName BoneName, float DamageAmount, const FHitResult& /*HitInfo*/)
{
    if (ZombieState == EZombieState::EZS_Dead)
    {
        return;
    }

    bool bLegNewlyCrippled = false;

    if (IsLeftLegBone(BoneName) && !bLeftLegCrippled)
    {
        LeftLegAccumulatedDamage += DamageAmount;

        if (LeftLegAccumulatedDamage >= LegCrippleDamageThreshold)
        {
            bLeftLegCrippled = true;
            bLegNewlyCrippled = true;
            UE_LOG(LogTemp, Log, TEXT("[HitZone] %s — left leg crippled"), *GetName());
        }
    }
    else if (IsRightLegBone(BoneName) && !bRightLegCrippled)
    {
        RightLegAccumulatedDamage += DamageAmount;

        if (RightLegAccumulatedDamage >= LegCrippleDamageThreshold)
        {
            bRightLegCrippled = true;
            bLegNewlyCrippled = true;
            UE_LOG(LogTemp, Log, TEXT("[HitZone] %s — right leg crippled"), *GetName());
        }
    }

    if (!bLegNewlyCrippled)
    {
        return;
    }

    if (GetCrippledLegCount() >= 2)
    {
        // Both legs crippled — enter full crawl via existing EnterCrawlerMode()
        // which calls ApplyCrawlerPhysics() (sets Health = 1, shrinks capsule).
        EnterCrawlerStateFromLegDamage();
    }
    else
    {
        // One crippled leg — limp. Reduce speed, stay upright.
        if (GetCharacterMovement() && ZombieState != EZombieState::EZS_Crawling)
        {
            GetCharacterMovement()->MaxWalkSpeed = ZombieConfig.ChaseSpeed * 0.6f;
        }

        UE_LOG(LogTemp, Log, TEXT("[HitZone] %s — limping after one crippled leg"), *GetName());
    }
}

void AZombieCharacter::EnterCrawlerStateFromLegDamage()
{
    if (ZombieState == EZombieState::EZS_Crawling || ZombieState == EZombieState::EZS_Dead)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("[HitZone] %s — entering crawler from leg damage"), *GetName());

    // EnterCrawlerMode calls SetZombieState(EZS_Crawling) and ApplyCrawlerPhysics().
    EnterCrawlerMode();
}

// ─────────────────────────────────────────────
// Death / Crawler
// ─────────────────────────────────────────────

void AZombieCharacter::HandleDeath(bool bWasHeadshot)
{
    // Roll crawler chance — headshots and already-crawling zombies skip this.
    if (!bWasHeadshot && ZombieState != EZombieState::EZS_Crawling)
    {
        if (FMath::FRand() < ZombieConfig.CrawlerChance)
        {
            EnterCrawlerMode();
            return;
        }
    }

    // Full death
    SetZombieState(EZombieState::EZS_Dead);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DetachFromControllerPendingDestroy();
    SetLifeSpan(5.f);
}

void AZombieCharacter::EnterCrawlerMode()
{
    SetZombieState(EZombieState::EZS_Crawling);
    ApplyCrawlerPhysics();
}

void AZombieCharacter::ApplyCrawlerPhysics()
{
    Health = 1.f;
    GetCharacterMovement()->MaxWalkSpeed = ZombieConfig.CrawlSpeed;
    GetCapsuleComponent()->SetCapsuleHalfHeight(40.f);
}

// ─────────────────────────────────────────────
// Melee
// ─────────────────────────────────────────────

void AZombieCharacter::PerformMeleeAttack()
{
    if (!HasAuthority()) return;
    if (ZombieState == EZombieState::EZS_Dead || ZombieState == EZombieState::EZS_Crawling) return;

    // Phase 1B: arm-disabled state is tracked but melee gating based on arm
    // state is a Phase 2 addition when attack montage routing is implemented.

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastMeleeTime < ZombieConfig.MeleeAttackRate) return;
    LastMeleeTime = Now;

    Multicast_PlayMeleeAttackMontage();

    const FVector MeleeOrigin = GetActorLocation() + GetActorForwardVector() * 60.f;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    GetWorld()->SweepMultiByChannel(
        Hits,
        MeleeOrigin,
        MeleeOrigin,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(ZombieConfig.MeleeSphereRadius),
        Params
    );

    for (const FHitResult& Hit : Hits)
    {
        AShooterGameCharacter* HitPlayer = Cast<AShooterGameCharacter>(Hit.GetActor());
        if (!HitPlayer) continue;

        UGameplayStatics::ApplyPointDamage(
            HitPlayer,
            ZombieConfig.MeleeDamage,
            GetActorLocation(),
            Hit,
            GetController(),
            this,
            UDamageType::StaticClass()
        );
    }
}

void AZombieCharacter::Multicast_PlayMeleeAttackMontage_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("Multicast_PlayMeleeAttackMontage fired on: %s | HasAuthority: %s"),
        *GetName(), HasAuthority() ? TEXT("Server") : TEXT("Client"));

    if (UZombieAnimInstance* ZombieAnim = Cast<UZombieAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        ZombieAnim->SetIsAttacking(true);
    }

    if (MeleeAttackMontage)
    {
        GetMesh()->GetAnimInstance()->Montage_Play(MeleeAttackMontage);
    }
    else
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AZombieCharacter::Multicast_PlayMeleeAttackMontage — MeleeAttackMontage null on %s"),
            *GetName());
    }
}

// ─────────────────────────────────────────────
// State
// ─────────────────────────────────────────────

void AZombieCharacter::SetZombieState(EZombieState NewState)
{
    ZombieState = NewState;
    OnRep_ZombieState();
}

void AZombieCharacter::OnRep_ZombieState()
{
    // Clients respond to state changes here.
    // Future: trigger client-side VFX, sounds, or anim state changes.
}

// ─────────────────────────────────────────────
// Health RepNotify
// ─────────────────────────────────────────────

void AZombieCharacter::OnRep_Health()
{
    // Future: update health bar or visual effects on clients.
}

// ─────────────────────────────────────────────
// Bone Side Helpers
// ─────────────────────────────────────────────

bool AZombieCharacter::IsLeftArmBone(FName BoneName)
{
    return BoneName == TEXT("upperarm_l")
        || BoneName == TEXT("lowerarm_l")
        || BoneName == TEXT("hand_l");
}

bool AZombieCharacter::IsRightArmBone(FName BoneName)
{
    return BoneName == TEXT("upperarm_r")
        || BoneName == TEXT("lowerarm_r")
        || BoneName == TEXT("hand_r");
}

bool AZombieCharacter::IsLeftLegBone(FName BoneName)
{
    return BoneName == TEXT("thigh_l")
        || BoneName == TEXT("calf_l")
        || BoneName == TEXT("foot_l")
        || BoneName == TEXT("ball_l");
}

bool AZombieCharacter::IsRightLegBone(FName BoneName)
{
    return BoneName == TEXT("thigh_r")
        || BoneName == TEXT("calf_r")
        || BoneName == TEXT("foot_r")
        || BoneName == TEXT("ball_r");
}

int32 AZombieCharacter::GetCrippledLegCount() const
{
    int32 Count = 0;
    if (bLeftLegCrippled)  ++Count;
    if (bRightLegCrippled) ++Count;
    return Count;
}

bool AZombieCharacter::AreBothArmsDisabled() const
{
    return bLeftArmDisabled && bRightArmDisabled;
}