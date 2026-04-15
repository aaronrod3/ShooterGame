// ZombieTypes.h
// Defines all zombie-specific enums and the FZombieConfig data struct.
// Used by AZombieCharacter, AZombieAIController, and AZombieSpawner.

#pragma once

#include "CoreMinimal.h"
#include "ZombieTypes.generated.h"


// -----------------------------------------------------------------------
// Zombie AI behavioral state — replicated on AZombieCharacter
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class EZombieState : uint8
{
    EZS_Wandering       UMETA(DisplayName = "Wandering"),       // No target, roaming
    EZS_Investigating   UMETA(DisplayName = "Investigating"),   // Heard something, moving to last known location
    EZS_Chasing         UMETA(DisplayName = "Chasing"),         // Has line-of-sight on a player, walking chase
    EZS_Sprinting       UMETA(DisplayName = "Sprinting"),       // Sprint-capable zombie, sprinting after player
    EZS_Attacking       UMETA(DisplayName = "Attacking"),       // Within melee range, executing attack
    EZS_Crawling        UMETA(DisplayName = "Crawling"),        // Downed — health set to 1, reduced speed
    EZS_Dead            UMETA(DisplayName = "Dead"),

    EZS_MAX             UMETA(Hidden)
};


// -----------------------------------------------------------------------
// Per-zombie configuration — set defaults here, override per BP subclass
// Exposed to editor so special zombie types (radioactive, etc.) can tweak
// -----------------------------------------------------------------------
USTRUCT(BlueprintType)
struct FZombieConfig
{
    GENERATED_BODY()

    // --- Movement ---

    // Normal walk/chase speed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Movement")
    float WalkSpeed = 50.f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Movement")
    float ChaseSpeed = 150.f; // Between walk and sprint — used during chase + investigate

    // Sprint speed — only used if bCanSprint is rolled true
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Movement")
    float SprintSpeed = 150.f;

    // Speed while in crawler/downed state
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Movement")
    float CrawlSpeed = 40.f;

    // --- Sprint Randomness ---

    // 0.0 = never sprints, 1.0 = always sprints. Rolled once on BeginPlay per zombie.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Sprint", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SprintChance = 0.25f;

    // --- Detection ---

    // Radius at which zombie can hear sounds (footsteps, gunshots)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Detection")
    float HearingRange = 1500.f;

    // Radius and half-angle for sight perception
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Detection")
    float SightRange = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Detection")
    float SightAngle = 45.f;   // Half-angle in degrees (total FOV = SightAngle * 2)

    // Distance at which zombie gives up chasing and returns to wander
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Detection")
    float DisengageDistance = 2800.f;

    // Distance at which zombie enters melee attack range
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Detection")
    float MeleeRange = 130.f;

    // --- Melee ---

    // Base damage per melee hit
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Melee")
    float MeleeDamage = 20.f;

    // Minimum seconds between melee attacks
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Melee")
    float MeleeAttackRate = 1.2f;

    // Sphere radius for melee overlap hit check
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Melee")
    float MeleeSphereRadius = 80.f;

    // --- Health ---

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Health")
    float MaxHealth = 100.f;

    // --- Crawler / Downed ---

    // 0.0 = never crawls, 1.0 = always crawls on death-hit (unless headshot).
    // Rolled at the moment the fatal hit lands.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Crawler", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CrawlerChance = 0.30f;
    
    // --- Idle / Wander ---

    // Minimum seconds the zombie stands idle between wander moves
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Wander")
    float MinIdleTime = 2.0f;

    // Maximum seconds the zombie stands idle between wander moves
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Wander")
    float MaxIdleTime = 6.0f;

    // --- Investigation ---

    // Seconds the zombie investigates LastKnownLocation before giving up and returning to wander
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float InvestigationTime = 15.0f;
};