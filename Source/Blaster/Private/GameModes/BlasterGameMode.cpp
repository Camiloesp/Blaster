// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"

void ABlasterGameMode::PlayerEliminated(ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Eliminated();
	}
}

void ABlasterGameMode::RequestRespawn(ACharacter* EliminatedCharacter, AController* EliminatedController)
{
	if (EliminatedCharacter)
	{
		EliminatedCharacter->Reset();	//APawn::Reset
		EliminatedCharacter->Destroy();
	}
	if (EliminatedController)
	{
		// Call GetAllActorsOfClass on begin play and have those references saved?
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(EliminatedController, PlayerStarts[Selection]); //GameMode::RestartPlayerAtPlayerStart
	}
}
