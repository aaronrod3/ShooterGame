// Source/ShooterGame/Types/AmmoType.h
#pragma once

#include "CoreMinimal.h"
#include "AmmoType.generated.h"

UENUM(BlueprintType)
enum class EAmmoType : uint8
{
	EAT_9mm		UMETA(DisplayName = "9mm"),
	EAT_45ACP	UMETA(DisplayName = ".45 ACP"),
	EAT_38SPL	UMETA(DisplayName = ".38 Special"),
	EAT_556		UMETA(DisplayName = "5.56"),
	EAT_762		UMETA(DisplayName = "7.62"),
	EAT_308		UMETA(DisplayName = ".308"),
	EAT_40mm	UMETA(DisplayName = "40mm"),
	EAT_12Gauge	UMETA(DisplayName = "12 Gauge"),

	EAT_MAX		UMETA(DisplayName = "DefaultMAX")
};