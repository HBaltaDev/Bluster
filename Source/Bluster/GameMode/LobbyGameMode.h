// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLUSTER_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UPROPERTY(EditAnywhere, Category = "PlayerNumber")
	int32 PlayerNumber{ 2 };

	UPROPERTY(EditAnywhere, Category = "Map")
	FString PathToLobby{ TEXT("/Game/Maps/BlasterMap") };
};
