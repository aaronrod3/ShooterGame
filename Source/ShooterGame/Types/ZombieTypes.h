// ZombieTypes.h
// Defines all zombie-specific enums and the FZombieConfig data struct.
// Used by AZombieCharacter, AZombieAIController, and AZombieSpawner.

#pragma once

#include "CoreMinimal.h"
#include "ZombieTypes.generated.h"



// -----------------------------------------------------------------------
// Spawn mode for AZombieSpawnManager
// -----------------------------------------------------------------------
UENUM(BlueprintType)
enum class ESpawnMode : uint8
{
    ESM_Single   UMETA(DisplayName = "Single"),   // Spawn exactly one zombie
    ESM_Group    UMETA(DisplayName = "Group"),    // Spawn exactly MinCount zombies
    ESM_Random   UMETA(DisplayName = "Random"),   // Spawn random count between MinCount and MaxCount

    ESM_MAX      UMETA(Hidden)
};



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
    
    // Radius within which this zombie will alert nearby wandering zombies
    // when it enters any non-Wandering state. Alerted zombies will move to
    // this zombie's location at ChaseSpeed via the Investigating state.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Detection")
    float AlertRadius = 1200.f;
    

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
    
    // Radius around the zombie's current location to pick a random wander destination
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Wander")
    float WanderRadius = 800.f;

    // Sphere radius used to find nearby zombies for horde cohesion during wandering.
    // Zombies within this distance contribute to the horde centroid calculation.
    // Set to 0 to disable cohesion entirely for this zombie type.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Wander", meta = (ClampMin = "0.0"))
    float HordeCohesionRadius = 800.f;

    // Minimum number of nearby zombies that must be found within HordeCohesionRadius
    // before cohesion bias is applied. If fewer are found, this zombie wanders freely.
    // e.g. 2 = needs at least 2 neighbors (not counting self) to form a horde.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Wander", meta = (ClampMin = "1"))
    int32 MinCohesionNeighbors = 2;

    // How strongly the wander destination is pulled toward the horde centroid when
    // the chosen point strays beyond HordeCohesionRadius.
    // 0.0 = no bias (full random wander), 1.0 = always move directly to centroid.
    // 0.6 is a good default — noticeable grouping without robotic lock-step movement.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Wander", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CohesionBiasFactor = 0.6f;
    
    
    
    // --- Investigation ---

    // Randomized investigation duration — zombie searches for a random time in this range
    // before giving up and returning to wander (spec: 12–17 seconds)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float MinInvestigationTime = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float MaxInvestigationTime = 17.0f;
    
    // radius around LastKnownLocation the zombie searches while investigating
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float InvestigateWanderRadius = 400.f;
    
    // --- Group Alert Investigation ---

    // How long a group-alerted zombie searches before giving up and returning
    // to Wandering — intentionally shorter than normal investigation so hordes
    // dissolve at a predictable rate when the player escapes the area.
    // Randomized per zombie within this range.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float MinAlertInvestigationTime = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float MaxAlertInvestigationTime = 10.0f;
    
    // Seconds after completing a group-alert investigation before this zombie
    // can be re-alerted by another broadcast. Prevents clustering zombies from
    // perpetually re-alerting each other after the player has escaped.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombie|Investigation")
    float ReAlertCooldown = 10.0f;
};