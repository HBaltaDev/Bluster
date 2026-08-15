// Fill out your copyright notice in the Description page of Project Settings.


#include "CombatComponent.h"
#include "../Weapon/Weapon.h"
#include "../BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include <Net/UnrealNetwork.h>

//#include "Bluster/HUD/BlasterHUD.h"
#include "Bluster/PlayerController/BlasterPlayerController.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	BaseWalkSpeed = 600.f;
	AimWalkSpeed = 450.f;

	// ...
}


void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
	DOREPLIFETIME(UCombatComponent, CombatState);
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	}

	if (Character->GetFollowCamera())
	{
		DefaultFOV = Character->GetFollowCamera()->FieldOfView;
		CurrentFOV = DefaultFOV;
	}
	
	if (Character->HasAuthority())
	{
		InitializeCarriedAmmo();
	}
}



// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	
	if (Character && Character->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;
		
		SetHUDCrosshairs(DeltaTime);
		
		InterpFOV(DeltaTime);
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{
	if (Character == nullptr || Character->Controller == nullptr)
	{
		return;
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	
	if (Controller)
	{
		HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
		
		if (HUD)
		{
			if (EquippedWeapon)
			{
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsLeft = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
				HUDPackage.CrosshairsTop = nullptr;
			}
			
			// Calculate crosshair spread

			// [0, 600] -> [0, 1]
			const FVector2D WalkSpeedRange(0.f, Character->GetCharacterMovement()->MaxWalkSpeed);
			const FVector2D VelocityMultiplierRange(0.f, 1.f);
			FVector Velocity = Character->GetVelocity();
			Velocity.Z = 0.f;

			CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

			if (Character->GetCharacterMovement()->IsFalling())
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
			}
			else
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
			}

			//HUDPackage.CrosshairSpread = CrosshairVelocityFactor + CrosshairInAirFactor;
			
			if (bAiming)
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
			}
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
			}

			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

			HUDPackage.CrosshairSpread = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + CrosshairShootingFactor;
			
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::InterpFOV(const float DeltaTime)
{
	if (EquippedWeapon == nullptr) return;

	if (bAiming)
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
	}
	if (Character && Character->GetFollowCamera())
	{
		Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;

	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
	}
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && Character)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
		}
		
		if (EquippedWeapon->EquipSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EquippedWeapon->EquipSound,
				Character->GetActorLocation()
			);
		}
		
		Character->GetCharacterMovement()->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = true;
	}
}

void UCombatComponent::FireButtonPressed(const bool bPressed)
{
	bFireButtonPressed = bPressed;
	
	if (bFireButtonPressed && IsValid(EquippedWeapon))
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	// bCanFire aşşağıdaki kontrolü yapmak için var. If bloğunda bool kontrol etmek aşşağıdaki kontrolden daha hızlı.
	// if (GetWorld()->GetTimerManager().IsTimerActive(FireTimer))
	// {
	// 	return;
	// }
	
	if (!CanFire())
	{
		return;
	}

	if (EquippedWeapon)
	{
		CrosshairShootingFactor = 0.75f;
	}
	
	ServerFire(HitTarget);

	bCanFire = false;

	GetWorld()->GetTimerManager().SetTimer(
		FireTimer,
		this,
		&UCombatComponent::FireTimerFinished,
		EquippedWeapon->FireDelay
	);
}

void UCombatComponent::FireTimerFinished()
{
	bCanFire = true;

	if (bFireButtonPressed && EquippedWeapon->bAutomatic)
	{
		Fire();
	}
	
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
}


void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if (!IsValid(EquippedWeapon) || !IsValid(Character) || CombatState != ECombatState::ECS_Unoccupied)
	{
		return;
	}
	
	Character->PlayFireMontage(bAiming);
	EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::Reload()
{
	if (CarriedAmmo > 0 && CombatState != ECombatState::ECS_Reloading)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;

	CombatState = ECombatState::ECS_Reloading;
	HandleReload();
}

void UCombatComponent::FinishReloading()
{
	if (Character == nullptr) return;
	if (Character->HasAuthority())
	{
		CombatState = ECombatState::ECS_Unoccupied;
		UpdateAmmoValues();
	}
	
	if (bFireButtonPressed)
	{
		Fire();
	}
}

void UCombatComponent::UpdateAmmoValues()
{
	if (Character == nullptr || EquippedWeapon == nullptr) return;
	int32 ReloadAmount = AmountToReload();
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	EquippedWeapon->AddAmmo(-ReloadAmount);
}

int32 UCombatComponent::AmountToReload()
{
	if (EquippedWeapon == nullptr) return 0;
	int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();

	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		int32 AmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
		int32 Least = FMath::Min(RoomInMag, AmountCarried);
		return FMath::Clamp(RoomInMag, 0, Least);
	}
	return 0;
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	// FVector2D ViewportSize;
	// if (GEngine && GEngine->GameViewport)
	// {
	// 	GEngine->GameViewport->GetViewportSize(ViewportSize);
	// }
	//
	// const FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	// FVector CrosshairWorldPosition;
	// FVector CrosshairWorldDirection;
	//
	// const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
	// 	UGameplayStatics::GetPlayerController(this, 0),
	// 	CrosshairLocation,
	// 	CrosshairWorldPosition,
	// 	CrosshairWorldDirection
	// );

	//if (bScreenToWorld)
	//{
		//const FVector Start = CrosshairWorldPosition;

		//const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
		
		// const bool bHit = GetWorld()->LineTraceSingleByChannel(
		// 	TraceHitResult,
		// 	Start,
		// 	End,
		// 	ECollisionChannel::ECC_Visibility
		// );
	//}
	
	if (!Character || !Character->GetController() || !GetWorld())
	{
		return;
	}
	
	FVector ViewPointLocation{};
	FRotator ViewPointRotation{};

	Character->GetController()->GetPlayerViewPoint(ViewPointLocation, ViewPointRotation);

	const FVector EndLocation = ViewPointLocation + ViewPointRotation.Vector() * TRACE_LENGTH;
	const FVector TraceDirection = ViewPointRotation.Vector();
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);
	Params.AddIgnoredActor(GetOwner());
	Params.bTraceComplex = false;
	
	// if (const AActor* OwnerActor = GetOwner())
	// {
	// 	Params.AddIgnoredActor(OwnerActor);
	// }
	
	FVector Start = ViewPointLocation;
	
	if (Character)
	{
		// alttaki 2 satır kod aynı şeyi yapıyor alttaki yön vector kullanılmak istenildiğinde durumda kullanılmalı
		const float DistanceToCharacter = FVector::Distance(Character->GetActorLocation(), ViewPointLocation);
		//const float DistanceToCharacter = (Character->GetActorLocation() - ViewPointLocation).Size();
		Start += ViewPointRotation.Vector() * (DistanceToCharacter + 100.f);
		//DrawDebugSphere(GetWorld(), Start, 16.f, 12.f, FColor::Red, false);
	}

	const bool bIsHit = GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, EndLocation, ECollisionChannel::ECC_Visibility, Params);
		
	if (!bIsHit)
	{
		TraceHitResult.ImpactPoint = EndLocation;
	}
	
	if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
	{
		HUDPackage.CrosshairColor = FLinearColor::Red;
	}
	else
	{
		HUDPackage.CrosshairColor = FLinearColor::White;
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (Character == nullptr || WeaponToEquip == nullptr)
	{
		return;
	}
	
	if (EquippedWeapon)
	{
		EquippedWeapon->Dropped();
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);

	const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
	}

	EquippedWeapon->SetOwner(Character);
	EquippedWeapon->SetHUDAmmo();
	
	if (CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}

	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
	
	if (EquippedWeapon->EquipSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			EquippedWeapon->EquipSound,
			Character->GetActorLocation()
		);
	}
	
	if (EquippedWeapon->IsEmpty())
	{
		Reload();
	}
	
	// Widget set null in changing weapon state
	//EquippedWeapon->ShowPickupWidget(false);

	Character->GetCharacterMovement()->bOrientRotationToMovement = false;
	Character->bUseControllerRotationYaw = true;
}

bool UCombatComponent::CanFire() const
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}
	
	return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->Controller) : Controller;
	if (Controller)
	{
		Controller->SetHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, StartingSMGAmmo);
	CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, StartingShotgunAmmo);
}

void UCombatComponent::OnRep_CombatState()
{
	switch (CombatState)
	{
	case ECombatState::ECS_Reloading:
		HandleReload();
		break;
		
	case ECombatState::ECS_Unoccupied:
		if (bFireButtonPressed)
		{
			Fire();
		}
		break;
	}
}

void UCombatComponent::HandleReload()
{
	Character->PlayReloadMontage();
}
