// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "BlasterTypes/Team.h"
#include "BlasterPlayerState.generated.h"


class ABlasterCharacter;
class ABlasterPlayerController;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Score() override;
	

		
	/* ABlasterPlayerState */

public:
	/*
	* Replication notifies
	*/
	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

private:

	UPROPERTY()
	ABlasterCharacter* Character; // Always initialize to nullptr. Pointers sometimes have garbage if not initialized. Another option is to add a UPROPERTY macro
	UPROPERTY()
	ABlasterPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;

	UPROPERTY( ReplicatedUsing = OnRep_Team )
	ETeam Team = ETeam::ET_NoTeam;

	UFUNCTION()
	void OnRep_Team();

public:

	/* Getters */
	FORCEINLINE ETeam GetTeam() const { return Team; }

	/* Setters */
	void SetTeam( ETeam NewTeam );
};
