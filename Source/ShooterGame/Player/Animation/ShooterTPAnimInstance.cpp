#include "ShooterTPAnimInstance.h"
#include "ShooterGame/Player/Character/ShooterGameCharacter.h"
#include "ShooterGame/Types/PlayerWeaponStance.h"

void UShooterTPAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!ShooterGameCharacter) return;

	UpdateTPData();
}

void UShooterTPAnimInstance::UpdateTPData()
{
	bUseTwoHandedGrip = (WeaponStance == EPlayerWeaponStance::EPWS_Rifle
					  || WeaponStance == EPlayerWeaponStance::EPWS_Shotgun);
}