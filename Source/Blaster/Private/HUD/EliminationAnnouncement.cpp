// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/EliminationAnnouncement.h"
#include "Components/TextBlock.h"
#include "PlayerState/BlasterPlayerState.h"

void UEliminationAnnouncement::SetEliminationAnnouncementText( FString AttackerName, FString VictimName, APlayerState* Attacker, APlayerState* Victim )
{
	ABlasterPlayerState* AttackerPlayerState = Cast<ABlasterPlayerState>( Attacker );
	ABlasterPlayerState* VictimPlayerState = Cast<ABlasterPlayerState>( Victim );
	if (!Attacker || !Victim) return;


	KillerNameText->SetText( FText::FromString( *AttackerName ) );
	VictimNameText->SetText( FText::FromString( *VictimName ) );

	// Attacker color
	if (AttackerPlayerState->GetTeam() == ETeam::ET_RedTeam)
	{
		KillerNameText->SetColorAndOpacity( FSlateColor( FColor::Red ) );
	}
	else if (AttackerPlayerState->GetTeam() == ETeam::ET_BlueTeam)
	{
		KillerNameText->SetColorAndOpacity( FSlateColor( FColor::Blue ) );
	}

	// Victim color
	if (VictimPlayerState->GetTeam() == ETeam::ET_RedTeam)
	{
		VictimNameText->SetColorAndOpacity( FSlateColor( FColor::Red ) );
	}
	else if (VictimPlayerState->GetTeam() == ETeam::ET_BlueTeam)
	{
		VictimNameText->SetColorAndOpacity( FSlateColor( FColor::Blue ) );
	}

	// Middle message. (i.e player Killed player. If suicide dont add middle message)
	if (AttackerPlayerState == VictimPlayerState)
	{
		AnnouncementText->SetText( FText::FromString( TEXT( " Took the easy way out..." ) ) );
		VictimNameText->SetText( FText::FromString( TEXT( "" ) ) );
	}
	else
	{
		AnnouncementText->SetText( FText::FromString( TEXT( " Killed " ) ) );
	}


	/*FString EliminationAnnouncementText = FString::Printf( TEXT( "%s Elimminated %s!" ), *AttackerName, *VictimName );
	if (AnnouncementText)
	{
		AnnouncementText->SetText( FText::FromString( EliminationAnnouncementText ) );
	}*/
}
