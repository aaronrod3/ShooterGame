// ZombieCharacter.cpp

#include "ZombieCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "ZombieAnimInstance.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

AZombieCharacter::AZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    bAlwaysRelevant = true;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;
    // RotationRate controls how fast the zombie turns to face target/movement direction
    GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
    bUseControllerRotationYaw = false;
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
        UE_LOG(LogTemp, Error, TEXT("ZombieCharacter: ZombieAIControllerClass not set — assign BP_ZombieAIController in BP_ZombieCharacter Class Defaults"));
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

    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    // Crawler instant kill — already at 1 HP, any hit kills it
    if (ZombieState == EZombieState::EZS_Crawling)
    {
        Health = 0.f;
        HandleDeath(false);
        return ActualDamage;
    }

    Health = FMath::Clamp(Health - DamageAmount, 0.f, ZombieConfig.MaxHealth);

    if (Health <= 0.f)
    {
        const bool bHeadshot = IsHeadshot(DamageEvent);
        HandleDeath(bHeadshot);
    }

    return ActualDamage;
}

bool AZombieCharacter::IsHeadshot(const FDamageEvent& DamageEvent) const
{
    // Point damage carries bone name — check if it's a head bone
    if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
        const FName HitBone = PointDamage->HitInfo.BoneName;

        // Standard humanoid head bone names — extend if your skeleton uses a different name
        static const TArray<FName> HeadBones = { TEXT("head"), TEXT("Head"), TEXT("neck_01") };
        return HeadBones.Contains(HitBone);
    }
    return false;
}

void AZombieCharacter::HandleDeath(bool bWasHeadshot)
{
    // Roll crawler chance — headshots skip this entirely
    if (!bWasHeadshot && ZombieState != EZombieState::EZS_Crawling)
    {
        const float Roll = FMath::FRand();
        if (Roll < ZombieConfig.CrawlerChance)
        {
            EnterCrawlerMode();
            return;
        }
    }

    // Full death
    SetZombieState(EZombieState::EZS_Dead);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Detach the AI controller so the BT stops immediately
    DetachFromControllerPendingDestroy();

    // Destroy after a delay (gives time for death animation to play)
    SetLifeSpan(5.f);
}

void AZombieCharacter::EnterCrawlerMode()
{
    SetZombieState(EZombieState::EZS_Crawling);
    ApplyCrawlerPhysics();
}

void AZombieCharacter::ApplyCrawlerPhysics()
{
    // Set health to 1 — crawler is a one-shot
    Health = 1.f;

    // Reduce movement speed to crawl speed
    GetCharacterMovement()->MaxWalkSpeed = ZombieConfig.CrawlSpeed;

    // Shrink capsule so the zombie appears low to the ground
    // Half-height reduced from default (~88) to crawl height (~40)
    GetCapsuleComponent()->SetCapsuleHalfHeight(40.f);
}

// ─────────────────────────────────────────────
// Melee
// ─────────────────────────────────────────────

void AZombieCharacter::PerformMeleeAttack()
{
    if (!HasAuthority()) return;
    if (ZombieState == EZombieState::EZS_Dead || ZombieState == EZombieState::EZS_Crawling) return;

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastMeleeTime < ZombieConfig.MeleeAttackRate) return;
    LastMeleeTime = Now;
    
    Multicast_PlayMeleeAttackMontage();

    // Sphere sweep at zombie's forward position
    const FVector MeleeOrigin = GetActorLocation() + GetActorForwardVector() * 60.f;

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    GetWorld()->SweepMultiByChannel(
        Hits,
        MeleeOrigin,
        MeleeOrigin,                          // Start == End = point sphere check
        FQuat::Identity,
        ECollisionChannel::ECC_Pawn,
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


// ─────────────────────────────────────────────
// Melee Anim — Multicast
// ─────────────────────────────────────────────

void AZombieCharacter::Multicast_PlayMeleeAttackMontage_Implementation()
{
    
    UE_LOG(LogTemp, Warning, TEXT("Multicast_PlayMeleeAttackMontage fired on: %s | HasAuthority: %s"),
    *GetName(),
    HasAuthority() ? TEXT("Server") : TEXT("Client"));
    
    
    // Runs on every client (and server). Drives the anim flag + plays the montage locally.
    UZombieAnimInstance* ZombieAnim = Cast<UZombieAnimInstance>(GetMesh()->GetAnimInstance());
    if (ZombieAnim)
    {
        ZombieAnim->SetIsAttacking(true);
    }

    if (MeleeAttackMontage)
    {
        GetMesh()->GetAnimInstance()->Montage_Play(MeleeAttackMontage);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AZombieCharacter::Multicast_PlayMeleeAttackMontage — MeleeAttackMontage is null on %s. Assign it in BP_ZombieCharacter Class Defaults."), *GetName());
    }
}

// ─────────────────────────────────────────────
// State
// ─────────────────────────────────────────────

void AZombieCharacter::SetZombieState(EZombieState NewState)
{
    // Server sets the value — OnRep_ZombieState fires on clients automatically
    ZombieState = NewState;
    OnRep_ZombieState();   // Call manually on server since RepNotify only auto-fires on clients
}

void AZombieCharacter::OnRep_ZombieState()
{
    // Clients respond to state changes here — anim BP reads ZombieState directly
    // Future: trigger client-side VFX, sounds, or anim state changes here
}

// ─────────────────────────────────────────────
// Health RepNotify
// ─────────────────────────────────────────────

void AZombieCharacter::OnRep_Health()
{
    // Clients: update HUD, health bar, or visual effects here in future steps
}