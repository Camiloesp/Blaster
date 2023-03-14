// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EliminationAnnouncement.generated.h"

class UHorizontalBox;
class UTextBlock;
class APlayerState;
/**
 * 
 */
UCLASS()
class BLASTER_API UEliminationAnnouncement : public UUserWidget
{
	GENERATED_BODY()
	
	/* UEliminationAnnouncement */
public:

	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* AnnouncementBox;
	UPROPERTY( meta = (BindWidget) )
	UTextBlock* AnnouncementText;

	UPROPERTY( meta = (BindWidget) )
	UTextBlock* KillerNameText;
	UPROPERTY( meta = (BindWidget) )
	UTextBlock* VictimNameText;

	void SetEliminationAnnouncementText( FString AttackerName, FString VictimName, APlayerState* Attacker, APlayerState* Victim ); // APlayerState* Attacker, APlayerState* Victim Cast<ABlasterPlayerState>( AttackerState );
};
