// ZombieCharacter.h
// Base class for all zombie enemies.
// Handles replicated health, EZombieState, TakeDamage with headshot detection,
// crawler transition (health set to 1), and melee attack sphere check.
// AI logic lives in AZombieAIController 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterGame/Types/ZombieTypes.h"
#include "ShooterGame/Types/FireMode.h"   // ETeam
#include "ZombieAIController.h"
#include "ZombieCharacter.generated.h"

class UZombieAnimInstance;

UCLASS()
class SHOOTERGAME_API AZombieCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AZombieCharacter();

    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PostInitializeComponents() override;

    // Overrides AActor::TakeDamage — entry point for all incoming damage
    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    // Called by AZombieAIController when inside melee range
    void PerformMeleeAttack();

    // ── Getters ──
    FORCEINLINE float           GetHealth()         const { return Health; }
    FORCEINLINE float           GetMaxHealth()      const { return ZombieConfig.MaxHealth; }
    FORCEINLINE EZombieState    GetZombieState()    const { return ZombieState; }
    FORCEINLINE ETeam           GetTeam()           const { return Team; }
    FORCEINLINE bool            GetCanSprint()      const { return bCanSprint; }
    FORCEINLINE bool            IsCrawler()         const { return ZombieState == EZombieState::EZS_Crawling; }
    FORCEINLINE FZombieConfig&  GetZombieConfig()         { return ZombieConfig; }

    // Called by AZombieAIController to sync state (drives anim blueprint)
    void SetZombieState(EZombieState NewState);

    // ── Config ──
    // Editable per BP subclass — special zombie types override defaults here
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Config")
    FZombieConfig ZombieConfig;

protected:
    virtual void BeginPlay() override;

private:
    
    // Assign BP_ZombieAIController in BP_ZombieCharacter's Class Defaults
    UPROPERTY(EditDefaultsOnly, Category = "Zombie|AI")
    TSubclassOf<AZombieAIController> ZombieAIControllerClass;
    
    // ── Health ──
    UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Zombie|Health")
    float Health = 100.f;

    UFUNCTION()
    void OnRep_Health();

    // ── State ──
    UPROPERTY(ReplicatedUsing = OnRep_ZombieState, VisibleAnywhere, Category = "Zombie|State")
    EZombieState ZombieState = EZombieState::EZS_Wandering;

    UFUNCTION()
    void OnRep_ZombieState();

    // ── Team ──
    UPROPERTY(VisibleAnywhere, Category = "Zombie|Team")
    ETeam Team = ETeam::ET_Zombies;

    // ── Sprint ──
    // Rolled once on BeginPlay — true means this zombie sprints immediately on sight
    UPROPERTY(Replicated, VisibleAnywhere, Category = "Zombie|Sprint")
    bool bCanSprint = false;

    // ── Melee cooldown ──
    float LastMeleeTime = -999.f;

    // ── Internal helpers ──
    void HandleDeath(bool bWasHeadshot);
    void EnterCrawlerMode();

    // Checks if the hit bone qualifies as a headshot
    bool IsHeadshot(const FDamageEvent& DamageEvent) const;

    // Applies crawler transition: sets health to 1, reduces speed, resizes capsule
    // Called server-side only
    void ApplyCrawlerPhysics();
};