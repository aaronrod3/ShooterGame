#pragma once

#include "CoreMinimal.h"
#include "PlayerWeaponStance.generated.h"

UENUM(BlueprintType)
enum class EPlayerWeaponStance : uint8
{
	EPWS_Unarmed	UMETA(DisplayName = "Unarmed"),
	EPWS_Pistol		UMETA(DisplayName = "Pistol"),
	EPWS_Rifle		UMETA(DisplayName = "Rifle"),
	EPWS_Shotgun	UMETA(DisplayName = "Shotgun"),
	EPWS_Other		UMETA(DisplayName = "Other")
};