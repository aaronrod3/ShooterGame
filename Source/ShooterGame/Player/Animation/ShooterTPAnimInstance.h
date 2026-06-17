#pragma once

#include "CoreMinimal.h"
#include "ShooterAnimInstanceBase.h"
#include "ShooterTPAnimInstance.generated.h"

/**
 * Third-person anim instance.
 * Inherits all shared state from UShooterAnimInstanceBase.
 * TP-specific state is added per phase as needed.
 * Phase 4: no TP-only variables — left-arm FABRIK alpha uses bWeaponEquipped
 *           from the base class directly in ABP_TP_Default.
 */
UCLASS()
class SHOOTERGAME_API UShooterTPAnimInstance : public UShooterAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	void UpdateTPData();
};