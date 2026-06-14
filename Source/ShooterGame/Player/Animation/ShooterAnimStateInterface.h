// Source/ShooterGame/Player/Animation/ShooterAnimStateInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterAnimStateInterface.generated.h"

/**
 * Animation-state communication interface.
 * Implemented by UShooterAnimInstanceBase so that Anim Notify States
 * can push state changes to the AnimInstance without a hard cast.
 *
 * All functions are BlueprintNativeEvent:
 *   - C++ implementation lives in UShooterAnimInstanceBase (_Implementation suffix)
 *   - A Blueprint AnimBP that derives from UShooterAnimInstanceBase can
 *     override any function in the graph using the BlueprintImplementableEvent path
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UShooterAnimStateInterface : public UInterface
{
    GENERATED_BODY()
};

class SHOOTERGAME_API IShooterAnimStateInterface
{
    GENERATED_BODY()

public:
    /**
     * Called by ANS_LeftHandGrip notify states to override left-hand
     * IK visibility and blend speed for the duration of a montage window.
     *
     * @param bIsLeftHandOnWeapon  True = hand should contact the weapon.
     *                             False = hand should pull away (e.g. reload).
     * @param BlendSpeed           Override interp speed for this blend only.
     *                             Pass 0.f to use the AnimBP's GripInterpSpeed default.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Anim|State")
    void UpdateLeftHandGrip(bool bIsLeftHandOnWeapon, float BlendSpeed);

    /**
     * Called by ANS_BlockADS notify states to block or unblock ADS entry
     * at the AnimInstance level, independent of CombatComponent tick state.
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Anim|State")
    void SetAimingBlocked(bool bBlocked);

    /**
     * General-purpose state message channel for future notify-driven events.
     * MessageTag is a name key (e.g. "Grip.Override", "Weapon.Lower").
     * Value is an optional float payload (0.f if unused).
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Anim|State")
    void OnAnimStateMessage(FName MessageTag, float Value);
};