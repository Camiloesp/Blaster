// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerState/BlasterPlayerState.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Net/UnrealNetwork.h"

void ABlasterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME( ABlasterPlayerState, Defeats );
	DOREPLIFETIME( ABlasterPlayerState, Team );
}


void ABlasterPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount);

	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());
		}
	}
}

void ABlasterPlayerState::OnRep_Score()
{
	Super::OnRep_Score();

	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
		{
			Controller->SetHUDScore(GetScore());// Score);
		}
	}
}

void ABlasterPlayerState::AddToDefeats(int32 DefeatsAmount) // Called from gamemode
{
	Defeats += DefeatsAmount; // OnRep_Defeats();

	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}

void ABlasterPlayerState::SetTeam( ETeam NewTeam )
{
	Team = NewTeam; // OnRep_Team
	ABlasterCharacter* BCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BCharacter)
	{
		BCharacter->SetTeamColor( Team );
	}
}

void ABlasterPlayerState::OnRep_Team()
{
	ABlasterCharacter* BCharacter = Cast<ABlasterCharacter>( GetPawn() );
	if (BCharacter)
	{
		BCharacter->SetTeamColor( Team );
	}
}

void ABlasterPlayerState::OnRep_Defeats()
{
	Character = Character ? Character : Cast<ABlasterCharacter>(GetPawn());
	if (Character)
	{
		Controller = Controller ? Controller : Cast<ABlasterPlayerController>(Character->Controller);
		if (Controller)
		{
			Controller->SetHUDDefeats(Defeats);
		}
	}
}