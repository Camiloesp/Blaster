// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "BlasterGameState.generated.h"

class ABlasterPlayerState;
/**
 * 
 */
UCLASS()
class BLASTER_API ABlasterGameState : public AGameState
{
	GENERATED_BODY()
	/* AGameState */

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	

	/* ABlasterGameState */
public:

	UPROPERTY(Replicated)
	TArray<ABlasterPlayerState*> TopScoringPlayers;

	/* 
	* Teams
	*/
	TArray<ABlasterPlayerState*> RedTeam;
	TArray<ABlasterPlayerState*> BlueTeam;

	UPROPERTY( ReplicatedUsing = OnRep_RedTeamScore )
	float RedTeamScore = 0.f;
	UPROPERTY( ReplicatedUsing = OnRep_BlueTeamScore )
	float BlueTeamScore = 0.f;
	UFUNCTION()
	void OnRep_RedTeamScore();
	UFUNCTION()
	void OnRep_BlueTeamScore();



	void UpdateTopScore(ABlasterPlayerState* ScoringPlayer);

protected:
private:

	float TopScore = 0.f;
};
