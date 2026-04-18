// Source/ShooterGame/Components/ShellAudioComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Sound/SoundCue.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "ShellAudioComponent.generated.h"

/**
 * UShellAudioComponent
 *
 * Shell casing impact audio by physical surface type.
 * Attach to BP_CaseEject_* actors and assign cues in the Details panel.
 * PlayImpactSound() is called from the casing's OnHit event.
 *
 * Renamed from UShellSFXCompomentS1 (SoundFX Studio 2023 plugin).
 * SUPPRESSEDWEAPONSIAPI → SHOOTERGAME_API.
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class SHOOTERGAME_API UShellAudioComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UShellAudioComponent();

    // -----------------------------------------------------------------------
    // Surface Impact Cues — assign in child BP_CaseEject_* Details panel
    // -----------------------------------------------------------------------

    UPROPERTY(EditAnywhere, Category = "ShellAudio|Cues")
    USoundCue* Cue_Metal = nullptr;

    UPROPERTY(EditAnywhere, Category = "ShellAudio|Cues")
    USoundCue* Cue_Concrete = nullptr;

    UPROPERTY(EditAnywhere, Category = "ShellAudio|Cues")
    USoundCue* Cue_Wood = nullptr;

    UPROPERTY(EditAnywhere, Category = "ShellAudio|Cues")
    USoundCue* Cue_Dirt = nullptr;

    UPROPERTY(EditAnywhere, Category = "ShellAudio|Cues")
    USoundCue* Cue_Default = nullptr;

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    /**
     * Play the casing impact sound matching the physical surface at impact.
     * Call this from the casing actor's OnHit or OnComponentHit event.
     * @param ImpactLocation  World location of the casing impact.
     * @param SurfaceType     Physical surface type from the hit result.
     */
    UFUNCTION(BlueprintCallable, Category = "ShellAudio")
    void PlayImpactSound(FVector ImpactLocation, EPhysicalSurface SurfaceType);

protected:
    virtual void BeginPlay() override;

private:
    USoundCue* GetCueForSurface(EPhysicalSurface SurfaceType) const;
};