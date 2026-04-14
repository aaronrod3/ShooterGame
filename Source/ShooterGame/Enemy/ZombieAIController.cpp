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
            SetZombieStateAndBB(EZombieState::EZS_Wandering);
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

    // During chase/sprint, rotate control toward the target — not just pawn forward
    AActor* ChaseTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (ChaseTarget &&
        (ZombieOwner->GetZombieState() == EZombieState::EZS_Chasing ||
         ZombieOwner->GetZombieState() == EZombieState::EZS_Sprinting))
    {
        FRotator LookAt = (ChaseTarget->GetActorLocation() - ZombieOwner->GetActorLocation()).Rotation();
        LookAt.Pitch = 0.f;
        LookAt.Roll  = 0.f;
        SetControlRotation(LookAt);
    }
    else
    {
        // Use velocity direction so zombie naturally faces where it's walking
        const FVector Velocity = ZombieOwner->GetVelocity();
        if (!Velocity.IsNearlyZero(1.f))
        {
            FRotator MovementFacing = Velocity.Rotation();
            MovementFacing.Pitch = 0.f;
            MovementFacing.Roll  = 0.f;
            SetControlRotation(MovementFacing);
        }
        // If velocity is zero (idle/standing still), hold current facing — no change
    }

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
            Blackboard->GetValueAsInt(BB_ZombieState)
        );
    }

    ValidateLineOfSight();
    CheckDisengage();

    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (Target)
    {
        const float Dist = FVector::Dist(ZombieOwner->GetActorLocation(), Target->GetActorLocation());
        const bool bInRange = Dist <= ZombieOwner->GetZombieConfig().MeleeRange;
        Blackboard->SetValueAsBool(BB_bIsInMeleeRange, bInRange);

        // Log every second so we can see the distance vs MeleeRange
        static float LastMeleeLog = -999.f;
        const float NowM = GetWorld()->GetTimeSeconds();
        if (NowM - LastMeleeLog >= 1.0f)
        {
            LastMeleeLog = NowM;
            UE_LOG(LogTemp, Warning, TEXT("[MELEE] Dist=%.0f | MeleeRange=%.0f | bInRange=%s | BBSet=%s"),
                Dist,
                ZombieOwner->GetZombieConfig().MeleeRange,
                bInRange ? TEXT("YES") : TEXT("NO"),
                Blackboard->GetValueAsBool(BB_bIsInMeleeRange) ? TEXT("YES") : TEXT("NO"));
        }
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
    
    UE_LOG(LogTemp, Warning, TEXT("[PERCEPTION] OnPerceptionUpdated fired — %d actors updated"), UpdatedActors.Num());

    for (AActor* Actor : UpdatedActors)
    {
        if (!Cast<AShooterGameCharacter>(Actor)) continue;

        FActorPerceptionBlueprintInfo PerceptionInfo;
        PerceptionComp->GetActorsPerception(Actor, PerceptionInfo);

        for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
        {
            if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
            {
                UE_LOG(LogTemp, Warning, TEXT("[PERCEPTION] Sight stimulus for %s — bSensed=%s"),
                    *Actor->GetName(), Stimulus.WasSuccessfullySensed() ? TEXT("TRUE") : TEXT("FALSE"));
                HandleSightStimulus(Actor, Stimulus.WasSuccessfullySensed());
            }
            else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
            {
                if (Stimulus.WasSuccessfullySensed())
                {
                    UE_LOG(LogTemp, Warning, TEXT("[PERCEPTION] Hearing stimulus for %s at location %s"),
                        *Actor->GetName(), *Stimulus.StimulusLocation.ToString());
                    HandleHearingStimulus(Actor, Stimulus.StimulusLocation);
                }
            }
        }
    }
}

void AZombieAIController::SetZombieStateAndBB(EZombieState NewState)
{
    if (!ZombieOwner || !Blackboard) return;
    ZombieOwner->SetZombieState(NewState);
    Blackboard->SetValueAsInt(BB_ZombieState, (int32)NewState);    // Int key — reliable cross-version
    UE_LOG(LogTemp, Warning, TEXT("[STATE] SetZombieStateAndBB -> %d"), (int32)NewState);
}

void AZombieAIController::HandleSightStimulus(AActor* SensedActor, bool bWasSensed)
{
    if (!Blackboard || !ZombieOwner) return;

    const float Now = GetWorld()->GetTimeSeconds();
    AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    const bool bInCooldown = (Now - TargetLostTime < ReacquireCooldown);

    UE_LOG(LogTemp, Warning, TEXT("[SIGHT] Actor=%s | bWasSensed=%s | CurrentTarget=%s | bInCooldown=%s (%.2fs since lost) | CurrentState=%d"),
        *SensedActor->GetName(),
        bWasSensed ? TEXT("TRUE") : TEXT("FALSE"),
        CurrentTarget ? *CurrentTarget->GetName() : TEXT("NULL"),
        bInCooldown ? TEXT("YES") : TEXT("NO"),
        Now - TargetLostTime,
        (int32)ZombieOwner->GetZombieState());

    if (bWasSensed)
    {
        if (bInCooldown)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SIGHT] BLOCKED by cooldown — skipping target acquisition"));
            return;
        }
        if (CurrentTarget)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SIGHT] Already has target — skipping"));
            return;
        }

        Blackboard->SetValueAsObject(BB_TargetActor, SensedActor);
        Blackboard->SetValueAsVector(BB_LastKnownLocation, SensedActor->GetActorLocation());

        const EZombieState NewState = ZombieOwner->GetCanSprint()
            ? EZombieState::EZS_Sprinting
            : EZombieState::EZS_Chasing;

        const float NewSpeed = ZombieOwner->GetCanSprint()
            ? ZombieOwner->GetZombieConfig().SprintSpeed
            : ZombieOwner->GetZombieConfig().ChaseSpeed;

        SetZombieStateAndBB(NewState);
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = NewSpeed;

        UE_LOG(LogTemp, Warning, TEXT("[SIGHT] *** TARGET ACQUIRED: %s | NewState=%d | Speed=%.0f ***"),
            *SensedActor->GetName(), (int32)NewState, NewSpeed);
    }
    else
    {
        if (Blackboard->GetValueAsObject(BB_TargetActor) == SensedActor)
        {
            UE_LOG(LogTemp, Warning, TEXT("[SIGHT] Perception lost target %s — calling LoseTarget"), *SensedActor->GetName());
            LoseTarget(SensedActor);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SIGHT] Lost non-target actor %s — ignoring"), *SensedActor->GetName());
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
    SetZombieStateAndBB(EZombieState::EZS_Investigating);
}

// ─────────────────────────────────────────────
// Line of Sight Validation (per-tick)
// ─────────────────────────────────────────────

void AZombieAIController::ValidateLineOfSight()
{
    if (!Blackboard || !ZombieOwner) return;

    const EZombieState State = ZombieOwner->GetZombieState();
    if (State != EZombieState::EZS_Chasing && 
    State != EZombieState::EZS_Sprinting && 
    State != EZombieState::EZS_Attacking) return;

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

    if (bBlocked)
    {
        if (LOSBlockedStartTime < 0.f)
        {
            LOSBlockedStartTime = GetWorld()->GetTimeSeconds();
            UE_LOG(LogTemp, Warning, TEXT("[LOS] LOS blocked — grace timer started"));
        }

        const float BlockedDuration = GetWorld()->GetTimeSeconds() - LOSBlockedStartTime;
        UE_LOG(LogTemp, Verbose, TEXT("[LOS] Still blocked — duration=%.2fs / grace=%.2fs"), BlockedDuration, LOSGracePeriod);

        if (BlockedDuration >= LOSGracePeriod)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LOS] Grace period expired (%.2fs) — calling LoseTarget"), BlockedDuration);
            LOSBlockedStartTime = -999.f;
            LoseTarget(Target);
        }
    }
    else
    {
        if (LOSBlockedStartTime >= 0.f)
        {
            UE_LOG(LogTemp, Warning, TEXT("[LOS] LOS restored — grace timer reset"));
        }
        LOSBlockedStartTime = -999.f;
        Blackboard->SetValueAsVector(BB_LastKnownLocation, Target->GetActorLocation());
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
        UE_LOG(LogTemp, Warning, TEXT("[DISENGAGE] Distance=%.0f > DisengageDistance=%.0f — calling LoseTarget"),
            Dist, ZombieOwner->GetZombieConfig().DisengageDistance);
        LoseTarget(Target);
    }
}

// ─────────────────────────────────────────────
// Centralized Target Loss
// ─────────────────────────────────────────────

void AZombieAIController::LoseTarget(AActor* LostTarget)
{
    LOSBlockedStartTime = -999.f;

    if (!Blackboard || !ZombieOwner) return;

    const float SpeedBefore = ZombieOwner->GetCharacterMovement()->MaxWalkSpeed;

    TargetLostTime = GetWorld()->GetTimeSeconds();
    Blackboard->SetValueAsVector(BB_LastKnownLocation, LostTarget->GetActorLocation());
    Blackboard->SetValueAsObject(BB_TargetActor, nullptr);
    SetZombieStateAndBB(EZombieState::EZS_Investigating);
    ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = ZombieOwner->GetZombieConfig().ChaseSpeed;

    UE_LOG(LogTemp, Warning, TEXT("[LOSETARGET] Lost=%s | SpeedBefore=%.0f | SpeedAfter=%.0f | TargetSet=NULL | State=Investigating"),
        *LostTarget->GetName(), SpeedBefore, ZombieOwner->GetCharacterMovement()->MaxWalkSpeed);
}

void AZombieAIController::OnInvestigateComplete()
{
    if (!ZombieOwner) return;

    const float SpeedBefore = ZombieOwner->GetCharacterMovement()->MaxWalkSpeed;

    SetZombieStateAndBB(EZombieState::EZS_Wandering);
    ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = ZombieOwner->GetZombieConfig().WalkSpeed;

    UE_LOG(LogTemp, Warning, TEXT("[INVESTIGATECOMPLETE] SpeedBefore=%.0f | SpeedAfter=%.0f | State=Wandering — WHO CALLED THIS?"),
        SpeedBefore, ZombieOwner->GetCharacterMovement()->MaxWalkSpeed);

    // This will print the C++ callstack in the output log
    UE_LOG(LogTemp, Warning, TEXT("[INVESTIGATECOMPLETE] CallStack follows:"));
    FDebug::DumpStackTraceToLog(ELogVerbosity::Warning);
}

// ─────────────────────────────────────────────
// Combat
// ─────────────────────────────────────────────

void AZombieAIController::TriggerMeleeAttack()
{
    if (!ZombieOwner) return;
    
    if (ZombieOwner->GetZombieState() == EZombieState::EZS_Attacking) return;
    
    SetZombieStateAndBB(EZombieState::EZS_Attacking);
    ZombieOwner->PerformMeleeAttack();

    // Return to chasing after the attack animation window (matches MeleeAttackRate)
    const float ReturnDelay = FMath::Max(ZombieOwner->GetZombieConfig().MeleeAttackRate, 1.0f);
    GetWorldTimerManager().ClearTimer(AttackReturnHandle); // cancel any previous
    GetWorldTimerManager().SetTimer(AttackReturnHandle, [this]()
    {
        if (!ZombieOwner || !Blackboard) return;
        AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
        if (CurrentTarget)
        {
            SetZombieStateAndBB(EZombieState::EZS_Chasing);
        }
    }, ReturnDelay, false);
}