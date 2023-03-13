// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BlasterTypes/Team.h"
#include "FlagZone.generated.h"

class USphereComponent;

UCLASS()
class BLASTER_API AFlagZone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFlagZone();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	/* AFlagZone */

public:

	UPROPERTY( EditAnywhere )
	ETeam Team;

protected:

	UFUNCTION()
	virtual void OnSphereOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult );

private:

	UPROPERTY(EditAnywhere)
	USphereComponent* ZoneSphere;

};
