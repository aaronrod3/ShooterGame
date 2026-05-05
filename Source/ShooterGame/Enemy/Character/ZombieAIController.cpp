#include "ZombieAIController.h"
#include "ZombieCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/OverlapResult.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

const FName AZombieAIController::BB_TargetActor                     = TEXT("TargetActor");
const FName AZombieAIController::BB_LastKnownLocation               = TEXT("LastKnownLocation");
const FName AZombieAIController::BB_ZombieState                     = TEXT("ZombieState");
const FName AZombieAIController::BB_bCanSprint                      = TEXT("bCanSprint");
const FName AZombieAIController::BB_bIsInMeleeRange                 = TEXT("bIsInMeleeRange");
const FName AZombieAIController::BB_bIsIdling                       = TEXT("bIsIdling");
const FName AZombieAIController::BB_bInvestigationTimerStarted      = TEXT("bInvestigationTimerStarted");
const FName AZombieAIController::BB_AlertSourceLocation             = TEXT("AlertSourceLocation");

AZombieAIController::AZombieAIController()
{
    PrimaryActorTick.bCanEverTick = true;

    PerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp"));
    SetPerceptionComponent(*PerceptionComp);

    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->DetectionByAffiliation.bDetectEnemies    = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals   = false;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;

    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 950.f;   // set a real default so CDO isn't 0
    HearingConfig->DetectionByAffiliation.bDetectEnemies    = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals   = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;

    // Configure BOTH senses here in constructor so BP CDO picks them up
    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->ConfigureSense(*HearingConfig);   // ← keep this

    PerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AZombieAIController::BeginPlay()
{
    Super::BeginPlay();

    if (ZombieOwner)
    {
        UpdatePerceptionConfig();
    }

    // Nuclear option: re-register this controller's listener with the
    // global perception system after BeginPlay — fixes placed instance bug
    if (UAIPerceptionSystem* PerceptionSystem =
        UAIPerceptionSystem::GetCurrent(GetWorld()))
    {
        PerceptionSystem->UpdateListener(*PerceptionComp);
    }
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
    
    // TEMP:
    UE_LOG(LogTemp, Warning, TEXT("[POSSESS] OnPerceptionUpdated bound — zombie: %s"),
        ZombieOwner ? *ZombieOwner->GetName() : TEXT("NULL"));

    if (ZombieBehaviorTree)
    {
        RunBehaviorTree(ZombieBehaviorTree);

        if (Blackboard)
        {
            Blackboard->SetValueAsBool(BB_bCanSprint, ZombieOwner->GetCanSprint());
            SetZombieStateAndBB(EZombieState::EZS_Wandering);
            Blackboard->SetValueAsBool(BB_bIsIdling, false);
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
    BroadcastGroupAlert(); 

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
    const FZombieConfig Config = ZombieOwner->GetZombieConfig();

    SightConfig->SightRadius = Config.SightRange;
    SightConfig->LoseSightRadius = Config.SightRange + 50.f;
    SightConfig->PeripheralVisionAngleDegrees = Config.SightAngle;
    SightConfig->SetMaxAge(5.0f);

    HearingConfig->HearingRange = Config.HearingRange;
    HearingConfig->SetMaxAge(3.0f);

    PerceptionComp->ConfigureSense(*SightConfig);
    PerceptionComp->ConfigureSense(*HearingConfig);

    // Force hearing into the listener whitelist — fixes UE bug where
    // hearing config set in constructor doesn't register with FPerceptionListener
    PerceptionComp->UpdatePerceptionAllowList(UAISense::GetSenseID<UAISense_Hearing>(), true);

    PerceptionComp->RequestStimuliListenerUpdate();

    // TEMP — verify hearing sense is live
    const UAISenseConfig* HearingCheck = PerceptionComp->GetSenseConfig(
        UAISense::GetSenseID<UAISense_Hearing>());

    UE_LOG(LogTemp, Warning,
        TEXT("[PERCEPTION CONFIG] Applied — HearingRange:%.0f SightRange:%.0f | HearingLive:%s"),
        Config.HearingRange,
        Config.SightRange,
        HearingCheck ? TEXT("YES") : TEXT("NO"));
}

// ─────────────────────────────────────────────
// Perception Events
// ─────────────────────────────────────────────

void AZombieAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors)
{
    if (!ZombieOwner || !Blackboard) return;

    UE_LOG(LogTemp, Warning, TEXT("[PERCEPTION] OnPerceptionUpdated fired — %d actors updated"),
        UpdatedActors.Num());

    for (AActor* Actor : UpdatedActors)
    {
        if (!Cast<AShooterGameCharacter>(Actor)) continue;

        FActorPerceptionBlueprintInfo PerceptionInfo;
        PerceptionComp->GetActorsPerception(Actor, PerceptionInfo);

        for (const FAIStimulus& Stimulus : PerceptionInfo.LastSensedStimuli)
        {
            if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
            {
                UE_LOG(LogTemp, Warning, TEXT("[PERCEPTION] Sight stimulus for %s bSensed:%s"),
                    *Actor->GetName(),
                    Stimulus.WasSuccessfullySensed() ? TEXT("TRUE") : TEXT("FALSE"));
                HandleSightStimulus(Actor, Stimulus.WasSuccessfullySensed());
            }
            else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
            {
                if (Stimulus.WasSuccessfullySensed())
                {
                    UE_LOG(LogTemp, Warning,
                        TEXT("[PERCEPTION] Hearing stimulus for %s at location %s"),
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

    const FZombieConfig& Config = ZombieOwner->GetZombieConfig();

    // Centralized speed assignment — every state transition goes through here
    switch (NewState)
    {
    case EZombieState::EZS_Wandering:
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = Config.WalkSpeed;
        break;

    case EZombieState::EZS_Investigating:
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = Config.ChaseSpeed;
        break;

    case EZombieState::EZS_Chasing:
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = Config.ChaseSpeed;
        break;

    case EZombieState::EZS_Sprinting:
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = Config.SprintSpeed;
        break;

    case EZombieState::EZS_Attacking:
        // Stop moving during attack — BT MoveTo will resume after attack window
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = 0.f;
        break;

    case EZombieState::EZS_Crawling:
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = Config.CrawlSpeed;
        break;

    case EZombieState::EZS_Dead:
        ZombieOwner->GetCharacterMovement()->MaxWalkSpeed = 0.f;
        break;

    default:
        break;
    }

    ZombieOwner->SetZombieState(NewState);
    Blackboard->SetValueAsInt(BB_ZombieState, (int32)NewState);

    UE_LOG(LogTemp, Warning, TEXT("[STATE] SetZombieStateAndBB -> %d | Speed=%.0f"),
        (int32)NewState, ZombieOwner->GetCharacterMovement()->MaxWalkSpeed);
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

        // Cancel idle dwell — zombie is leaving wander state
        GetWorldTimerManager().ClearTimer(IdleDwellHandle);
        Blackboard->SetValueAsBool(BB_bIsIdling, false);
        
        // If this zombie was group-alerted, it now has a real target — clear the flag
        // so if it later loses the player it gets a full investigation window
        bIsGroupAlerted = false;

        // Cancel any active investigation timer — player re-acquired
        if (GetWorldTimerManager().IsTimerActive(InvestigationTimerHandle))
        {
            GetWorldTimerManager().ClearTimer(InvestigationTimerHandle);
            Blackboard->SetValueAsBool(BB_bInvestigationTimerStarted, false);
            
            UE_LOG(LogTemp, Warning, TEXT("[INVESTIGATE] Investigation timer cancelled — target re-acquired"));
        }

        Blackboard->SetValueAsObject(BB_TargetActor, SensedActor);
        Blackboard->SetValueAsVector(BB_LastKnownLocation, SensedActor->GetActorLocation());

        const EZombieState NewState = ZombieOwner->GetCanSprint()
            ? EZombieState::EZS_Sprinting
            : EZombieState::EZS_Chasing;

        SetZombieStateAndBB(NewState);
        UE_LOG(LogTemp, Warning, TEXT("[SIGHT] *** TARGET ACQUIRED: %s | NewState=%d ***"),
    *SensedActor->GetName(), (int32)NewState);
        
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
    UE_LOG(LogTemp, Warning, TEXT("[HEARING] HandleHearingStimulus called — SoundLocation: %s"),
        *SoundLocation.ToString());
    
    if (!Blackboard || !ZombieOwner) return;

    // Don't override an active chase/sprint with a hearing event
    const AActor* CurrentTarget = Cast<AActor>(Blackboard->GetValueAsObject(BB_TargetActor));
    if (CurrentTarget) return;

    // Also don't restart investigation if already investigating — just update the target location
    const EZombieState CurrentState = ZombieOwner->GetZombieState();
    if (CurrentState == EZombieState::EZS_Investigating)
    {
        Blackboard->SetValueAsVector(BB_LastKnownLocation, SoundLocation);
        UE_LOG(LogTemp, Warning, TEXT("[HEARING] Already investigating — updated LastKnownLocation to sound at %s"), *SoundLocation.ToString());
        return;
    }

    Blackboard->SetValueAsVector(BB_LastKnownLocation, SoundLocation);
    Blackboard->SetValueAsVector(BB_AlertSourceLocation, SoundLocation);
    SetZombieStateAndBB(EZombieState::EZS_Investigating);
    

    UE_LOG(LogTemp, Warning, TEXT("[HEARING] Sound heard at %s — entering Investigating"),
    *SoundLocation.ToString());
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

    // Only disengage during active chase — never interrupt an attack or investigation
    const EZombieState CurrentState = ZombieOwner->GetZombieState();
    if (CurrentState != EZombieState::EZS_Chasing &&
        CurrentState != EZombieState::EZS_Sprinting)
    {
        return;
    }

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

    // Cancel idle dwell — zombie is leaving wander state
    GetWorldTimerManager().ClearTimer(IdleDwellHandle);
    if (Blackboard) Blackboard->SetValueAsBool(BB_bIsIdling, false);

    if (!Blackboard || !ZombieOwner) return;

    const float SpeedBefore = ZombieOwner->GetCharacterMovement()->MaxWalkSpeed;

    TargetLostTime = GetWorld()->GetTimeSeconds();
    Blackboard->SetValueAsVector(BB_LastKnownLocation, LostTarget->GetActorLocation());
    Blackboard->SetValueAsObject(BB_TargetActor, nullptr);
    SetZombieStateAndBB(EZombieState::EZS_Investigating);

    UE_LOG(LogTemp, Warning, TEXT("[LOSETARGET] Lost=%s | SpeedBefore=%.0f | State=Investigating"),
    *LostTarget->GetName(), SpeedBefore);
}

void AZombieAIController::OnInvestigateComplete()
{
    if (!ZombieOwner || !Blackboard) return;

    GetWorldTimerManager().ClearTimer(InvestigationTimerHandle);

    // Clear both keys — releases the BT investigate branch entirely
    Blackboard->SetValueAsBool(BB_bInvestigationTimerStarted, false);
    Blackboard->ClearValue(BB_LastKnownLocation);
    
    bIsGroupAlerted = false;
    LastInvestigateCompleteTime = GetWorld()->GetTimeSeconds(); 

    const float SpeedBefore = ZombieOwner->GetCharacterMovement()->MaxWalkSpeed;
    SetZombieStateAndBB(EZombieState::EZS_Wandering);

    UE_LOG(LogTemp, Warning, TEXT("[INVESTIGATECOMPLETE] SpeedBefore=%.0f | State=Wandering"), SpeedBefore);
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

// ─────────────────────────────────────────────
// Idle Dwell (Wander pause)
// ─────────────────────────────────────────────

void AZombieAIController::StartIdleDwell()
{
    if (!ZombieOwner || !Blackboard) return;

    // Guard: only start an idle dwell while wandering
    if (ZombieOwner->GetZombieState() != EZombieState::EZS_Wandering) return;

    // Clear any previous dwell timer that may still be running
    GetWorldTimerManager().ClearTimer(IdleDwellHandle);

    const FZombieConfig& Config = ZombieOwner->GetZombieConfig();
    const float DwellTime = FMath::FRandRange(Config.MinIdleTime, Config.MaxIdleTime);

    // Tell the BT the zombie is now idling — wander movement will pause
    Blackboard->SetValueAsBool(BB_bIsIdling, true);

    GetWorldTimerManager().SetTimer(IdleDwellHandle, this,
        &AZombieAIController::OnIdleDwellComplete, DwellTime, false);

    UE_LOG(LogTemp, Warning, TEXT("[IDLE] StartIdleDwell — dwelling for %.2fs"), DwellTime);
}

void AZombieAIController::OnIdleDwellComplete()
{
    if (!ZombieOwner || !Blackboard) return;

    // Clear the timer handle and release the BT gate
    GetWorldTimerManager().ClearTimer(IdleDwellHandle);
    Blackboard->SetValueAsBool(BB_bIsIdling, false);

    UE_LOG(LogTemp, Warning, TEXT("[IDLE] OnIdleDwellComplete — resuming wander"));
}



// ─────────────────────────────────────────────
// Investigation Timer
// ─────────────────────────────────────────────

void AZombieAIController::StartInvestigationTimer()
{
    if (!ZombieOwner || !Blackboard) return;
    if (ZombieOwner->GetZombieState() != EZombieState::EZS_Investigating) return;

    GetWorldTimerManager().ClearTimer(InvestigationTimerHandle);

    const FZombieConfig& Config = ZombieOwner->GetZombieConfig();

    // Group-alerted zombies use a shorter window — they didn't see the player
    // directly so they give up sooner, letting the horde dissolve naturally
    const float InvestigationTime = bIsGroupAlerted
        ? FMath::FRandRange(Config.MinAlertInvestigationTime, Config.MaxAlertInvestigationTime)
        : FMath::FRandRange(Config.MinInvestigationTime,      Config.MaxInvestigationTime);

    Blackboard->SetValueAsBool(BB_bInvestigationTimerStarted, true);

    GetWorldTimerManager().SetTimer(InvestigationTimerHandle, this,
        &AZombieAIController::OnInvestigateComplete, InvestigationTime, false);

    UE_LOG(LogTemp, Warning, TEXT("[INVESTIGATE] StartInvestigationTimer — will expire in %.2fs | GroupAlert=%s"),
        InvestigationTime,
        bIsGroupAlerted ? TEXT("YES") : TEXT("NO"));
}


// ─────────────────────────────────────────────
// Group Alert — receive broadcast from a nearby alerted zombie
// ─────────────────────────────────────────────

void AZombieAIController::BroadcastGroupAlert()
{
    if (!ZombieOwner || !Blackboard) return;
    if (!ZombieOwner->HasAuthority()) return;

    const EZombieState CurrentState = ZombieOwner->GetZombieState();
    if (CurrentState == EZombieState::EZS_Wandering ||
        CurrentState == EZombieState::EZS_Dead       ||
        CurrentState == EZombieState::EZS_Crawling)
    {
        return;
    }

    // Throttle — only run the sphere overlap every 0.5s per zombie.
    // Firing every tick is redundant and expensive with large hordes.
    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastBroadcastTime < 0.5f) return;
    LastBroadcastTime = Now;

    const FVector MyLocation  = ZombieOwner->GetActorLocation();
    const float   AlertRadius = ZombieOwner->GetZombieConfig().AlertRadius;

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(ZombieOwner);

    GetWorld()->OverlapMultiByObjectType(
        Overlaps,
        MyLocation,
        FQuat::Identity,
        FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn),
        FCollisionShape::MakeSphere(AlertRadius),
        QueryParams
    );

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AZombieCharacter* NearbyZombie = Cast<AZombieCharacter>(Overlap.GetActor());
        if (!NearbyZombie) continue;

        // Only pull in purely wandering zombies — never interrupt an engaged one
        if (NearbyZombie->GetZombieState() != EZombieState::EZS_Wandering) continue;

        AZombieAIController* NearbyController =
            Cast<AZombieAIController>(NearbyZombie->GetController());
        if (!NearbyController) continue;

        NearbyController->ReceiveGroupAlert(MyLocation);
    }
}

void AZombieAIController::ReceiveGroupAlert(const FVector& AlerterLocation)
{
    if (!ZombieOwner || !Blackboard) return;
    if (!ZombieOwner->HasAuthority()) return;

    const EZombieState CurrentState = ZombieOwner->GetZombieState();
    if (CurrentState != EZombieState::EZS_Wandering) return;

    // Don't re-alert a zombie that just finished investigating — give it time
    // to wander away from the cluster before it can be pulled in again
    const float Now = GetWorld()->GetTimeSeconds();
    const float CooldownRemaining = ZombieOwner->GetZombieConfig().ReAlertCooldown
                                    - (Now - LastInvestigateCompleteTime);
    if (CooldownRemaining > 0.f)
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("[GROUP ALERT] %s blocked by re-alert cooldown (%.1fs remaining)"),
            *ZombieOwner->GetName(), CooldownRemaining);
        return;
    }

    // Cancel idle dwell so the zombie moves immediately
    GetWorldTimerManager().ClearTimer(IdleDwellHandle);
    Blackboard->SetValueAsBool(BB_bIsIdling, false);

    Blackboard->SetValueAsVector(BB_LastKnownLocation, AlerterLocation);
    bIsGroupAlerted = true;
    SetZombieStateAndBB(EZombieState::EZS_Investigating);

    UE_LOG(LogTemp, Warning,
        TEXT("[GROUP ALERT] %s received alert — moving to alerter at %s"),
        *ZombieOwner->GetName(),
        *AlerterLocation.ToString());
}


void AZombieAIController::SetObjectiveDestination(const FVector& Destination)
{
    if (!ZombieOwner || !Blackboard) return;
    if (!ZombieOwner->HasAuthority()) return;

    // Cancel any idle dwell so the zombie moves immediately on spawn
    GetWorldTimerManager().ClearTimer(IdleDwellHandle);
    Blackboard->SetValueAsBool(BB_bIsIdling, false);

    // Push the objective location into the same BB key the investigate branch
    // already reads — BTTask_InvestigateWander and BTTask_WanderToPoint both
    // use this key, so no BT changes are needed.
    Blackboard->SetValueAsVector(BB_LastKnownLocation, Destination);

    // Use the same state transition path every other routing call uses.
    // EZS_Investigating → BT moves zombie to BB_LastKnownLocation →
    // BTTask calls StartInvestigationTimer() on arrival →
    // OnInvestigateComplete() fires → EZS_Wandering + full perception restored.
    SetZombieStateAndBB(EZombieState::EZS_Investigating);

    UE_LOG(LogTemp, Log,
        TEXT("[OBJECTIVE ROUTE] %s routed to objective dest: %s"),
        *ZombieOwner->GetName(),
        *Destination.ToString());
}