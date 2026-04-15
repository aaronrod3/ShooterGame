#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "ShooterGame/Types/ZombieTypes.h"
#include "ZombieAIController.generated.h"

class AZombieCharacter;
class UBehaviorTree;
class UBlackboardComponent;

UCLASS()
class SHOOTERGAME_API AZombieAIController : public AAIController
{
    GENERATED_BODY()

public:
    AZombieAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Zombie|AI")
    void TriggerMeleeAttack();
    
    UFUNCTION(BlueprintCallable, Category = "Zombie|AI")
    void OnInvestigateComplete();

    UFUNCTION(BlueprintCallable, Category = "Zombie|AI")
    void StartIdleDwell();
    
    UFUNCTION(BlueprintCallable, Category = "Zombie|AI")
    void StartInvestigationTimer();
    
    
    static const FName BB_TargetActor;
    static const FName BB_LastKnownLocation;
    static const FName BB_ZombieState;
    static const FName BB_bCanSprint;
    static const FName BB_bIsInMeleeRange;
    static const FName BB_bIsIdling;
    static const FName BB_bInvestigationTimerStarted;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActors);
    
  

private:
    UPROPERTY(VisibleAnywhere, Category = "Zombie|AI")
    UAIPerceptionComponent* PerceptionComp;

    UPROPERTY(VisibleAnywhere, Category = "Zombie|AI")
    UAISenseConfig_Sight* SightConfig;

    UPROPERTY(VisibleAnywhere, Category = "Zombie|AI")
    UAISenseConfig_Hearing* HearingConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Zombie|AI")
    UBehaviorTree* ZombieBehaviorTree;

    AZombieCharacter* ZombieOwner = nullptr;

    float TargetLostTime    = -999.f;
    float ReacquireCooldown = 0.3f;

    float LOSBlockedStartTime = -999.f;
    float LOSGracePeriod      = 1.5f;

    void SetZombieStateAndBB(EZombieState NewState);
    void UpdatePerceptionConfig();
    void HandleSightStimulus(AActor* SensedActor, bool bWasSensed);
    void HandleHearingStimulus(AActor* SensedActor, const FVector& SoundLocation);
    void ValidateLineOfSight();
    void CheckDisengage();
    void LoseTarget(AActor* LostTarget);

    FTimerHandle AttackReturnHandle;
    FTimerHandle IdleDwellHandle;
    FTimerHandle InvestigationTimerHandle;

    
    void OnIdleDwellComplete();

    
};