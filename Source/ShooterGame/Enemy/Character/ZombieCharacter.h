// Source/ShooterGame/Enemy/Character/ZombieCharacter.h
// Base class for all zombie enemies.
// Handles replicated health, EZombieState, TakeDamage with hit-zone routing,
// crawler transition (from leg damage or roll), and melee attack sphere check.
// AI logic lives in AZombieAIController.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ShooterGame/Types/ZombieTypes.h"
#include "ShooterGame/Types/HitZoneTypes.h"
#include "ShooterGame/Types/FireMode.h"
#include "ShooterGame/Components/HitZoneComponent.h"
#include "ZombieAIController.h"
#include "Engine/DamageEvents.h"
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

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        AController* EventInstigator, AActor* DamageCauser) override;

    void PerformMeleeAttack();

    // ── Getters ──
    FORCEINLINE float              GetHealth()            const { return Health; }
    FORCEINLINE float              GetMaxHealth()         const { return ZombieConfig.MaxHealth; }
    FORCEINLINE EZombieState       GetZombieState()       const { return ZombieState; }
    FORCEINLINE ETeam              GetTeam()              const { return Team; }
    FORCEINLINE bool               GetCanSprint()         const { return bCanSprint; }
    FORCEINLINE bool               IsCrawler()            const { return ZombieState == EZombieState::EZS_Crawling; }
    FORCEINLINE FZombieConfig&     GetZombieConfig()      { return ZombieConfig; }
    FORCEINLINE UHitZoneComponent* GetHitZoneComponent()  const { return HitZoneComponent; }

    // Called by AZombieAIController to sync state (drives anim blueprint)
    void SetZombieState(EZombieState NewState);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "HitZone")
    TObjectPtr<UHitZoneComponent> HitZoneComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Config")
    FZombieConfig ZombieConfig;

    UPROPERTY(EditDefaultsOnly, Category = "Zombie|Anim")
    UAnimMontage* MeleeAttackMontage = nullptr;

protected:
    virtual void BeginPlay() override;

    // ── Hit Zone State ──

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    bool bHeadDestroyed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    bool bLeftArmDisabled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    bool bRightArmDisabled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    bool bLeftLegCrippled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    bool bRightLegCrippled = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    float LeftArmAccumulatedDamage = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    float RightArmAccumulatedDamage = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    float LeftLegAccumulatedDamage = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zombie|HitZone")
    float RightLegAccumulatedDamage = 0.f;

    // ── Hit Zone Tuning ── (override per zombie BP subclass)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|HitZone")
    float TorsoStaggerDuration = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|HitZone")
    float ArmDisableDamageThreshold = 30.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|HitZone")
    float LegCrippleDamageThreshold = 25.f;

    // Minimum scaled damage required to confirm a lethal headshot.
    // Set above a single pistol headshot value on tougher zombie variants
    // to require two headshots (e.g. armoured skull).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|HitZone")
    float HeadshotKillDamageThreshold = 20.f;

    // ── Hit Zone Handlers ──

    void HandleZombieHitZoneDamage(EHitZone HitZone, FName BoneName, float DamageAmount,
        const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser);

    void HandleZombieHeadshot(EHitZone HitZone, FName BoneName, float DamageAmount,
        const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser);

    void HandleZombieTorsoHit(float DamageAmount, const FHitResult& HitInfo);
    void HandleZombieArmHit(FName BoneName, float DamageAmount, const FHitResult& HitInfo);
    void HandleZombieLegHit(FName BoneName, float DamageAmount, const FHitResult& HitInfo);

    bool IsLeftArmBone(FName BoneName)  const;
    bool IsRightArmBone(FName BoneName) const;
    bool IsLeftLegBone(FName BoneName)  const;
    bool IsRightLegBone(FName BoneName) const;

    int32 GetCrippledLegCount() const;
    bool  AreBothArmsDisabled() const;
    void  EnterCrawlerStateFromLegDamage();

private:

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
    UPROPERTY(Replicated, VisibleAnywhere, Category = "Zombie|Sprint")
    bool bCanSprint = false;

    // ── Melee cooldown ──
    float LastMeleeTime = -999.f;

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayMeleeAttackMontage();
    void Multicast_PlayMeleeAttackMontage_Implementation();

    // ── Death / Crawler ──
    void HandleDeath(bool bWasHeadshot);
    void EnterCrawlerMode();
    void ApplyCrawlerPhysics();
};