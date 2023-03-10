// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/BlasterGameMode.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PlayerState/BlasterPlayerState.h"
#include "GameState/BlasterGameState.h"

namespace MatchState
{
	const FName Cooldown = FName( "Cooldown" );
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

void ABlasterGameMode::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime; //126
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
			SetMatchState( MatchState::Cooldown );
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			// Server travel only works for all players in a packaged build.
			RestartGame();
		}
	}
}

void ABlasterGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>( *It );
		if (BlasterPlayer)
		{
			BlasterPlayer->OnMatchStateSet( MatchState, bTeamsMatch );
		}
	}
}

float ABlasterGameMode::CalculateDamage( AController* Attacker, AController* Victim, float BaseDamage )
{
	return BaseDamage;
}

void ABlasterGameMode::PlayerEliminated( ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController )
{
	ABlasterPlayerState* AttackerPlayerState = AttackerController ? Cast<ABlasterPlayerState>( AttackerController->PlayerState ) : nullptr;
	ABlasterPlayerState* VictimPlayerState = VictimController ? Cast<ABlasterPlayerState>( VictimController->PlayerState ) : nullptr;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && BlasterGameState)
	{
		TArray<ABlasterPlayerState*> PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : BlasterGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add( LeadPlayer );
		}

		AttackerPlayerState->AddToScore( 1.f );
		BlasterGameState->UpdateTopScore( AttackerPlayerState );
		if (BlasterGameState->TopScoringPlayers.Contains( AttackerPlayerState ))
		{
			ABlasterCharacter* Leader = Cast<ABlasterCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if (!BlasterGameState->TopScoringPlayers.Contains( PlayersCurrentlyInTheLead[i] ))
			{
				ABlasterCharacter* Loser = Cast<ABlasterCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}

	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats( 1.f );
	}

	if (EliminatedCharacter)
	{
		EliminatedCharacter->Eliminated( false );
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ABlasterPlayerController* BlasterPlayer = Cast<ABlasterPlayerController>( *It );
		if (BlasterPlayer && AttackerPlayerState && VictimPlayerState)
		{
			BlasterPlayer->BroadcastElimination( AttackerPlayerState, VictimPlayerState );
		}
	}
}

void ABlasterGameMode::RequestRespawn( ACharacter* EliminatedCharacter, AController* EliminatedController )
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
		UGameplayStatics::GetAllActorsOfClass( this, APlayerStart::StaticClass(), PlayerStarts );
		int32 Selection = FMath::RandRange( 0, PlayerStarts.Num() - 1 );
		RestartPlayerAtPlayerStart( EliminatedController, PlayerStarts[Selection] ); //GameMode::RestartPlayerAtPlayerStart
	}
}

void ABlasterGameMode::PlayerLeftGame( ABlasterPlayerState* PlayerLeaving )
{
	if (!PlayerLeaving) return;

	ABlasterGameState* BlasterGameState = GetGameState<ABlasterGameState>();
	if (BlasterGameState && BlasterGameState->TopScoringPlayers.Contains( PlayerLeaving ))
	{
		BlasterGameState->TopScoringPlayers.Remove( PlayerLeaving );
	}

	ABlasterCharacter* CharacterLeaving = Cast<ABlasterCharacter>( PlayerLeaving->GetPawn() );
	if (CharacterLeaving)
	{
		CharacterLeaving->Eliminated( true );
	}
}
