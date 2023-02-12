#pragma once // Prevents including this header file more than once in a given header file.

// Scoped enum
UENUM(BlueprintType)
enum class ETurningInPlace : uint8
{
	ETIP_Left UMETA(DisplayName = "Turning Left"),
	ETIP_Right UMETA(DisplayName = "Turning Right"),
	ETIP_NotTurning UMETA(DisplayName = "Not Turning"),

	ETIP_MAX UMETA(DisplayName = "DefaultMAX")
};
