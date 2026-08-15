// Fill out your copyright notice in the Description page of Project Settings.


#include "HitScanWeapon.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Bluster/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"
#include "DrawDebugHelpers.h"

void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	
	if (OwnerPawn == nullptr)
	{
		return;
	}
	AController* InstigatorController = OwnerPawn->GetController();

	if (const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash"))
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 1.25f;

		FHitResult FireHit;
		
		WeaponTraceHit(Start, HitTarget, FireHit);
		
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		if (BlasterCharacter && HasAuthority() && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(
					BlasterCharacter,
					Damage,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
		}
				
		if (ImpactParticleSystem)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactParticleSystem, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());	
		}
				
		if (ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ImpactParticles,
				FireHit.ImpactPoint,
				FireHit.ImpactNormal.Rotation()
			);
		}
				
		if (HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				HitSound,
				FireHit.ImpactPoint
			);
		}
		
		UWorld* World = GetWorld();
		
		if (World)
		{
			World->LineTraceSingleByChannel(
				FireHit,
				Start,
				End,
				ECollisionChannel::ECC_Visibility
			);
			
			if (FireHit.bBlockingHit)
			{
				if (MuzzleFlash)
				{
					UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, MuzzleFlash, SocketTransform.GetLocation(), SocketTransform.GetLocation().Rotation());	
				}
				
				if (FireSound)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						FireSound,
						GetActorLocation()
					);
				}
			}
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	// DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	// DrawDebugSphere(GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true);
	// DrawDebugLine(
	// 	GetWorld(),
	// 	TraceStart,
	// 	FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
	// 	FColor::Cyan,
	// 	true);

	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();
	if (World)
	{
		FVector End = bUseScatter ? TraceEndWithScatter(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 1.25f;

		World->LineTraceSingleByChannel(
			OutHit,
			TraceStart,
			End,
			ECollisionChannel::ECC_Visibility
		);
		
		FVector BeamEnd = End;
		
		if (OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		
		if (BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				BeamParticles,
				TraceStart,
				FRotator::ZeroRotator,
				true
			);
			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}

		if (BeamParticleSystem)
		{
			const FRotator BeamRotation =
				(BeamEnd - TraceStart).Rotation() + FRotator(0.f, 180.f, 0.f);

			UNiagaraComponent* Beam = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				BeamParticleSystem,
				TraceStart,
				BeamRotation
			);

			if (Beam)
			{
				Beam->SetVariableVec3(FName("User.Target"), BeamEnd);
			}
		}
	}
}
