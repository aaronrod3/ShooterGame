#include "ShooterTPAnimInstance.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"

void UShooterTPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!ShooterGameCharacter) return;

	UpdateTPData();
}

void UShooterTPAnimInstance::UpdateTPData()
{
	// Phase 4: no TP-only state to compute.
	// bWeaponEquipped (base class) drives left-arm FABRIK alpha in ABP_TP_Default.
	// Phase 5 will add weapon-config-driven state here.
}