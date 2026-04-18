// Source/ShooterGame/Components/AudioPerceptionComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AudioPerceptionComponent.generated.h"

/**
 * UAudioPerceptionComponent
 *
 * Universal "this actor made a sound" component. Add it to ANY actor that
 * produces gameplay-relevant noise — weapons, case ejectors, footsteps,
 * explosions, doors, etc.
 *
 * What it does:
 *   1. Calls UAISense_Hearing::ReportNoiseEvent at the emit location so all
 *      nearby AZombieAIControllers with UAISenseConfigHearing immediately
 *      receive a hearing stimulus and enter EZSInvestigating state.
 *   2. Performs an optional occlusion line-trace — if a wall is between
 *      the sound source and the emission point the effective radius is
 *      reduced by OcclusionRadiusReduction (0.0 = full block, 1.0 = no effect).
 *   3. Runs server-authoritative so only the server drives AI reactions,
 *      matching the existing ZombieAIController authority pattern.
 *
 * Usage:
 *   - Add to any actor via CreateDefaultSubobject in its constructor
 *   - Tune DefaultEmitRadius and OcclusionRadiusReduction per actor in editor
 *   - Call EmitSoundEvent() after any sound fires on that actor
 *   - Optionally pass an OverrideRadius for one-off louder/quieter events
 *
 * Multiplayer:
 *   Server RPC so owning clients trigger emission through the server,
 *   keeping AI reactions authoritative and cheat-proof.
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UAudioPerceptionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAudioPerceptionComponent();

    // -----------------------------------------------------------------------
    // Editor-Tunable Properties
    // -----------------------------------------------------------------------

    // How far this sound travels in open air (unreal units).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundPerception")
    float DefaultEmitRadius = 1500.f;

    // How far a suppressed shot travels (unreal units).
    // Zombies outside this radius will not hear suppressed fire.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundPerception")
    float SuppressedNoiseRadius = 450.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundPerception")
    bool bUseOcclusion = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundPerception",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float OcclusionRadiusReduction = 0.3f;

    // Loudness for unsuppressed shots. Scales against zombie HearingRange.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundPerception",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Loudness = 1.0f;

    // Loudness for suppressed shots. Lower value = shorter effective zombie reaction range.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SoundPerception",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SuppressedLoudness = 0.3f;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * Emit a sound event at the owner's current location.
     * Automatically routes through a Server RPC when called on a client.
     * @param OverrideRadius  If > 0, uses this instead of DefaultEmitRadius.
     *                        Useful for suppressed shots (smaller radius).
     * @param OverrideLoudness If > 0, uses this instead of Loudness.
     */
    UFUNCTION(BlueprintCallable, Category = "SoundPerception")
    void EmitSoundEvent(float OverrideRadius = -1.f, float OverrideLoudness = -1.f);

protected:
    virtual void BeginPlay() override;

private:
    // -----------------------------------------------------------------------
    // Internal emission — server-only, runs the actual ReportNoiseEvent
    // -----------------------------------------------------------------------
    void EmitSoundEvent_Internal(float Radius, float InLoudness);

    // Returns true if there is clear line of sight between Start and End.
    // Used to determine occlusion — if blocked, radius is reduced.
    bool HasLineOfSight(const FVector& Start, const FVector& End) const;

    // -----------------------------------------------------------------------
    // Multiplayer RPCs
    // -----------------------------------------------------------------------

    // Called by owning client to trigger emission on the server
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_EmitSoundEvent(float Radius, float InLoudness);
    void Server_EmitSoundEvent_Implementation(float Radius, float InLoudness);
    bool Server_EmitSoundEvent_Validate(float Radius, float InLoudness);
};