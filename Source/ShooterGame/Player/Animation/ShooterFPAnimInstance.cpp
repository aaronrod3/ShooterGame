#include "ShooterFPAnimInstance.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Components/CombatComponent.h"
#include "ShooterGame/Framework/ShooterGame.h"

void UShooterFPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!ShooterGameCharacter) return;

	UpdateFPData(DeltaSeconds);
}

void UShooterFPAnimInstance::UpdateFPData(float DeltaSeconds)
{
	if (!CombatComponent) return;

	// CombatComponent owns the ADS interp — read the result directly.
	// bIsAimingBlocked gate is already applied inside CombatComponent
	// before GetADSAlpha() is computed, so no second gate needed here.
	ADSAlpha = CombatComponent->GetADSAlpha();
}