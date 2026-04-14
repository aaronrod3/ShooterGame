#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

const FName AZombieAIController::BB_TargetActor       = TEXT("TargetActor");
const FName AZombieAIController::BB_LastKnownLocation = TEXT("LastKnownLocation");
const FName AZombieAIController::BB_ZombieState       = TEXT("ZombieState");
const FName AZombieAIController::BB_bCanSprint        = TEXT("bCanSprint");
const FName AZombieAIController::BB_bIsInMeleeRange   = TEXT("bIsInMeleeRange");

AZombieAIController::AZombieAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SetPerceptionComponent(*PerceptionComp);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals   = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    PerceptionComp->ConfigureSense(*SightConfig);

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->DetectionByAffiliation.bDetectEnemies    = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals   = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
    PerceptionComp->ConfigureSense(*HearingConfig);

    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ZombieOwner = Cast<AZombieCharacter>(InPawn);
    if (!ZombieOwner)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombieAIController: Cast to AZombieCharacter FAILED"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("ZombieAIController: OnPossess — BT valid: %s"),
        ZombieBehaviorTree ? TEXT("YES") : TEXT("NO"));

    UpdatePerceptionConfig();
    PerceptionComp->OnPerceptionUpdated.AddDynamic(this, &AZombieAIController::OnPerceptionUpdated);

    if (ZombieBehaviorTree)
    {
        RunBehaviorTree(ZombieBehaviorTree);

        if (Blackboard)
        {
            Blackboard->SetValueAsBool(BB_bCanSprint, ZombieOwner->GetCanSprint());
            Blackboard->SetValueAsEnum(BB_ZombieState, (uint8)EZombieState::EZS_Wandering);
        }
    }

    SetControlRotation(ZombieOwner->GetActorRotation());
}

void AZombieAIController::OnUnPossess()
{
    PerceptionComp->OnPerceptionUpdated.RemoveDynamic(this, &AZombieAIController::OnPerceptionUpdated);
    Super::OnUnPossess();
}

void AZombieAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!ZombieOwner || !Blackboard) return;

    FRotator PawnFacing = ZombieOwner->GetActorForwardVector().Rotation();
    PawnFacing.Pitch = 0.f;
    PawnFacing.Roll  = 0.f;
    SetControlRotation(PawnFacing);

    // Log state every 1 second
    static float LastLog = -999.f;
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastLog >= 1.0f)
    {
        LastLog = Now;
        AActor* T = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
        UE_LOG(LogTemp, Warning, TEXT("[TICK] State=%d | Target=%s | BBState=%d"),
            (int32)ZombieOwner->GetZombieState(),
            T ? *T->GetName() : TEXT("NULL"),
            (int32)Blackboard->GetValueAsEnum(BB_ZombieState)
        );
    }

    ValidateLineOfSight();
    CheckDisengage();

    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (Target)
    {
        const float Dist = FVector::Dist(ZombieOwner->GetActorLocation(), Target->GetActorLocation());
        Blackboard->SetValueAsBool(BB_bIsInMeleeRange, Dist <= ZombieOwner->GetZombieConfig().MeleeRange);
    }
    else
    {
        Blackboard->SetValueAsBool(BB_bIsInMeleeRange, false);
    }
}

// ─────────────────────────────────────────────
// Perception Config
// ─────────────────────────────────────────────

void AZombieAIController::UpdatePerceptionConfig()
{
    if (!ZombieOwner) return;

    const FZombieConfig& Config = ZombieOwner->GetZombieConfig();

    SightConfig->SightRadius                  = Config.SightRange;
    SightConfig->LoseSightRadius              = Config.SightRange + 50.f;
    SightConfig->PeripheralVisionAngleDegrees = Config.SightAngle;
    SightConfig->SetMaxAge(5.0f);

    HearingConfig->HearingRange = Config.HearingRange;
    HearingConfig->SetMaxAge(3.0f);

    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->ConfigureSense(*HearingConfig);
    PerceptionComp->RequestStimuliListenerUpdate();
}

// ─────────────────────────────────────────────
// Perception Events
// ─────────────────────────────────────────────

void AZombieAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    if (!ZombieOwner || !Blackboard) return;

    for (AActor* Actor : UpdatedActors)
    {
        if (!Cast<AShooterGameCharacter>(Actor)) continue;

        FActorPerceptionBlueprintInfo PerceptionInfo;
        PerceptionComp->GetActorsPerception(Actor, PerceptionInfo);

        for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
        {
            if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
            {
                HandleSightStimulus(Actor, Stimulus.WasSuccessfullySensed());
            }
            else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
            {
                if (Stimulus.WasSuccessfullySensed())
                {
                    HandleHearingStimulus(Actor, Stimulus.StimulusLocation);
                }
            }
        }
    }
}

void AZombieAIController::HandleSightStimulus(AActor* SensedActor, bool bWasSensed)
{
    if (!Blackboard || !ZombieOwner) return;

    const float Now = GetWorld()->GetTimeSeconds();

    UE_LOG(LogTemp, Warning, TEXT(">>> HandleSightStimulus: Actor=%s | bWasSensed=%s | TimeSinceLost=%.2f | Cooldown=%.2f"),
        *SensedActor->GetName(),
        bWasSensed ? TEXT("TRUE") : TEXT("FALSE"),
        Now - TargetLostTime,
        ReacquireCooldown);

    if (bWasSensed)
    {
        const bool bInCooldown = (Now - TargetLostTime < ReacquireCooldown);
        AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));

        UE_LOG(LogTemp, Warning, TEXT("    bInCooldown=%s | CurrentTarget=%s"),
            bInCooldown ? TEXT("YES — BLOCKED") : TEXT("NO"),
            CurrentTarget ? *CurrentTarget->GetName() : TEXT("NULL"));

        if (bInCooldown) return;
        if (CurrentTarget) return;

        Blackboard->SetValueAsObject(BB_TargetActor, SensedActor);
        Blackboard->SetValueAsVector(BB_LastKnownLocation, SensedActor->GetActorLocation());

        const EZombieState NewState = ZombieOwner->GetCanSprint()
            ? EZombieState::EZS_Sprinting
            : EZombieState::EZS_Chasing;

        ZombieOwner->SetZombieState(NewState);
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = ZombieOwner->GetCanSprint()
            ? ZombieOwner->GetZombieConfig().SprintSpeed
            : ZombieOwner->GetZombieConfig().WalkSpeed;

        UE_LOG(LogTemp, Warning, TEXT("    *** TARGET ACQUIRED: %s ***"), *SensedActor->GetName());
    }
    else
    {
        if (Blackboard->GetValueAsObject(BB_TargetActor) == SensedActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("    *** PERCEPTION LOST TARGET — calling LoseTarget ***"));
            LoseTarget(SensedActor);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("    Lost sight of non-target actor — ignoring"));
        }
    }
}

void AZombieAIController::HandleHearingStimulus(AActor* SensedActor, const FVector& SoundLocation)
{
    if (!Blackboard || !ZombieOwner) return;

    // Don't override an active chase with a hearing event
    const AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (CurrentTarget) return;

    Blackboard->SetValueAsVector(BB_LastKnownLocation, SoundLocation);
    ZombieOwner->SetZombieState(EZombieState::EZS_Investigating);
}

// ─────────────────────────────────────────────
// Line of Sight Validation (per-tick)
// ─────────────────────────────────────────────

void AZombieAIController::ValidateLineOfSight()
{
    if (!Blackboard || !ZombieOwner) return;

    const EZombieState State = ZombieOwner->GetZombieState();
    if (State != EZombieState::EZS_Chasing && State != EZombieState::EZS_Sprinting) return;

    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (!Target) return;

    FHitResult HitResult;
    const FVector Start = ZombieOwner->GetActorLocation() + FVector(0.f, 0.f, 64.f);
    const FVector End   = Target->GetActorLocation()      + FVector(0.f, 0.f, 64.f);

    FCollisionQueryParams Params;
    Params.AddIgnoredActor(ZombieOwner);
    Params.AddIgnoredActor(Target);

    const bool bBlocked = GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECollisionChannel::ECC_Visibility, Params
    );

    UE_LOG(LogTemp, Verbose, TEXT("ValidateLineOfSight: bBlocked=%s | HitActor=%s"),
        bBlocked ? TEXT("YES") : TEXT("NO"),
        (bBlocked && HitResult.GetActor()) ? *HitResult.GetActor()->GetName() : TEXT("none"));

    if (bBlocked)
    {
        UE_LOG(LogTemp, Warning, TEXT("ValidateLineOfSight: LOS BLOCKED by %s — calling LoseTarget"),
            HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("unknown"));
        LoseTarget(Target);
    }
}

// ─────────────────────────────────────────────
// Disengage (distance check)
// ─────────────────────────────────────────────

void AZombieAIController::CheckDisengage()
{
    if (!Blackboard || !ZombieOwner) return;

    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (!Target) return;

    const float Dist = FVector::Dist(ZombieOwner->GetActorLocation(), Target->GetActorLocation());
    if (Dist > ZombieOwner->GetZombieConfig().DisengageDistance)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombieAIController: Target exceeded disengage distance — losing target"));
        LoseTarget(Target);
    }
}

// ─────────────────────────────────────────────
// Centralized Target Loss
// ─────────────────────────────────────────────

void AZombieAIController::LoseTarget(AActor* LostTarget)
{
    if (!Blackboard || !ZombieOwner) return;

    TargetLostTime = GetWorld()->GetTimeSeconds();
    Blackboard->SetValueAsVector(BB_LastKnownLocation, LostTarget->GetActorLocation());
    Blackboard->SetValueAsObject(BB_TargetActor, nullptr);
    ZombieOwner->SetZombieState(EZombieState::EZS_Investigating);

    // Keep chase speed while investigating — zombie doesn't know player is gone yet
    // Speed drops to WalkSpeed only once Investigating resolves to Wandering
    ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = ZombieOwner->GetZombieConfig().ChaseSpeed;
}

void AZombieAIController::OnInvestigateComplete()
{
    if (!ZombieOwner) return;
    ZombieOwner->SetZombieState(EZombieState::EZS_Wandering);
    ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = ZombieOwner->GetZombieConfig().WalkSpeed;
}

// ─────────────────────────────────────────────
// Combat
// ─────────────────────────────────────────────

void AZombieAIController::TriggerMeleeAttack()
{
    if (ZombieOwner)
    {
        ZombieOwner->PerformMeleeAttack();
    }
}