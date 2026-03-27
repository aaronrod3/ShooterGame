#pragma once

#include "CoreMinimal.h"
#include "FireMode.generated.h"

UENUM(BlueprintType)
enum class EFireMode : uint8
{
	EFM_Safe		UMETA(DisplayName = "Safe"),
	EFM_SemiAuto	UMETA(DisplayName = "Semi Auto"),
	EFM_Burst		UMETA(DisplayName = "Burst"),
	EFM_FullAuto	UMETA(DisplayName = "Full Auto"),

	EFM_Max			UMETA(DisplayName = "DefaultMax")
};