// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterGameMode.h"
#include "../BlasterCharacter.h"
#include "Bluster/GameState/BlasterGameState.h"
#include "Bluster/PlayerController/BlasterPlayerController.h"
#include "Bluster/PlayerState/BlasterPlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ABlasterGameMode::ABlasterGameMode()
{
	bDelayedStart = true;
}

void ABlasterGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ABlasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	} 
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
	
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>(*It))
		{
			BlasterPlayer->OnMatchStateSet(MatchState);
		}
	}
}

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* ElimmedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	if (ElimmedCharacter)
	{
		if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
		if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
		ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>(AttackerController->PlayerState) : nullptr;
		ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>(VictimController->PlayerState) : nullptr;
		ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
		
		
		if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
		{
			AttackerPlayerState->AddToScore(1.f);
			BlasterGameState->UpdateTopScore(AttackerPlayerState);
		}
		
		if (VictimPlayerState)
		{
			VictimPlayerState->AddToDefeats(1);
		}
		
		ElimmedCharacter->Elim();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy();
	}
	
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		const int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	
		
		// I will need it when gamemode need to lunch code at BlasterCharacter class
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(ElimmedController->GetPawn());
		
		if (IsValid(BlasterCharacter))
		{
			BlasterCharacter->RespawnFromGameMode();
		}
	}
}