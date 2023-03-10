// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameModes/BlasterGameMode.h"
#include "TeamsGameMode.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API ATeamsGameMode : public ABlasterGameMode
{
	GENERATED_BODY()
	
public:
	ATeamsGameMode();

	virtual void PostLogin( APlayerController* NewPlayer ) override;
	virtual void Logout( AController* Exiting ) override;

protected:

	virtual void HandleMatchHasStarted() override;


	/* ATeamsGameMode */
public:

	virtual void PlayerEliminated( ABlasterCharacter* EliminatedCharacter, ABlasterPlayerController* VictimController, ABlasterPlayerController* AttackerController ) override;
	virtual float CalculateDamage( AController* Attacker, AController* Victim, float BaseDamage ) override;

protected:
private:
};
