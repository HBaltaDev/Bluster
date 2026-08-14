// Fill out your copyright notice in the Description page of Project Settings.


#include "ProjectileBullet.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectileBullet::AProjectileBullet()
{
	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->SetIsReplicated(true);
}

void AProjectileBullet::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// UE_LOG(
	// LogTemp,
	// Warning,
	// TEXT(
	// 	"OnHit | Authority: %s | OtherActor: %s | OtherComp: %s | ObjectType: %d"
	// ),
	// HasAuthority() ? TEXT("TRUE") : TEXT("FALSE"),
	// *GetNameSafe(OtherActor),
	// *GetNameSafe(OtherComp),
	// OtherComp
	// 	? static_cast<int32>(OtherComp->GetCollisionObjectType())
	// 	: -1);
	AController* InstigatorController = GetInstigatorController();
	// UE_LOG(
	// 	 LogTemp,
	// 	 Warning,
	// 	 TEXT("Damage | Target: %s | Instigator: %s | CanBeDamaged: %s"),
	// 	 *GetNameSafe(OtherActor),
	// 	 *GetNameSafe(InstigatorController),
	// 	 OtherActor->CanBeDamaged() ? TEXT("TRUE") : TEXT("FALSE")
	//  );
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (OwnerCharacter)
	{
		AController* OwnerController = OwnerCharacter->Controller;
		if (OwnerController)
		{
			UGameplayStatics::ApplyDamage(OtherActor, Damage, OwnerController, this, UDamageType::StaticClass());
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
