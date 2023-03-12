// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EliminationAnnouncement.generated.h"

class UHorizontalBox;
class UTextBlock;
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

	void SetEliminationAnnouncementText( FString AttackerName, FString VictimName );
};
