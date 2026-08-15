#pragma once

#define TRACE_LENGTH 80000.f

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher UMETA(DisplayName = "Rocket Launcher"),
	EWT_SMG UMETA(DisplayName = "SMG"),
	EWT_Shotgun UMETA(DisplayName = "Shotgun"),
	
	EWT_MAX UMETA(DisplayName = "DefaultMAX")
};
