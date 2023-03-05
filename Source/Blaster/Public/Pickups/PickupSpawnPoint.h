// Fill out your copyright notice in the Description page of Project Settings.

/*
	The purpose of this actor is to spawn Pickups
*/
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupSpawnPoint.generated.h"

class APickup;

UCLASS()
class BLASTER_API APickupSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	APickupSpawnPoint();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick( float DeltaTime ) override;


	/* APickupSpawnPoint */
public:

protected:

	UPROPERTY( EditAnywhere )
	TArray<TSubclassOf<APickup>> PickupClasses;

	UPROPERTY()
	APickup* SpawnedPickup;

	void SpawnPickup();
	void SpawnPickupTimerFinished();
	UFUNCTION()
	void StartSpawnPickupTimer(AActor* DestroyedActor);

private:

	FTimerHandle SpawnPickupTimer;
	UPROPERTY( EditAnywhere )
	float SpawnPickupTimeMin;
	UPROPERTY( EditAnywhere )
	float SpawnPickupTimeMax;
};
