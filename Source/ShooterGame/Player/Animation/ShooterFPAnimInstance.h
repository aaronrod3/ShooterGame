#pragma once

#include "CoreMinimal.h"
#include "ShooterAnimInstanceBase.h"
#include "ShooterFPAnimInstance.generated.h"

/**
 * First-person anim instance.
 * Inherits the full Phase 1/2 state contract from UShooterAnimInstanceBase.
 * FP-specific additions: ADSAlpha float, FP update hook for per-frame FP data.
 *
 * Phase 3 scope: thin stub + ADSAlpha.
 * Phase 4 will add procedural sway/lean offsets and recoil accumulation here.
 */
UCLASS()
class SHOOTERGAME_API UShooterFPAnimInstance : public UShooterAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// -----------------------------------------------------------------------
	// FP-only state
	// -----------------------------------------------------------------------

	/**
	 * [0,1] ADS blend alpha.
	 * Read directly from CombatComponent::GetADSAlpha() each tick.
	 * CombatComponent owns the interpolation — no second interp here.
	 * ADSInterpSpeed and the bIsAimingBlocked gate are therefore removed.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|FP|ADS")
	float ADSAlpha = 0.f;


private:
	void UpdateFPData(float DeltaSeconds);
};