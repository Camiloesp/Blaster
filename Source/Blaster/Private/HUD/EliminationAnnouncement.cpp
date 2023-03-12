// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/EliminationAnnouncement.h"
#include "Components/TextBlock.h"

void UEliminationAnnouncement::SetEliminationAnnouncementText( FString AttackerName, FString VictimName )
{
	FString EliminationAnnouncementText = FString::Printf( TEXT( "%s Elimminated %s!" ), *AttackerName, *VictimName );
	if (AnnouncementText)
	{
		AnnouncementText->SetText( FText::FromString( EliminationAnnouncementText ) );
	}
}
