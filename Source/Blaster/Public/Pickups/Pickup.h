// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pickup.generated.h"

class USphereComponent;
class USoundCue;
class UNiagaraComponent;
class UNiagaraSystem;

UCLASS()
class BLASTER_API APickup : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickup();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void Destroyed() override;


	/* APickup */

protected:

	UFUNCTION()
	virtual void OnSphereOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult );
	
	UPROPERTY( EditAnywhere )
	float BaseTurnRate = 45.f;

private:

	UPROPERTY( EditAnywhere )
	USphereComponent* OverlapSphere;

	UPROPERTY( EditAnywhere )
	USoundCue* PickupSound;

	UPROPERTY( EditAnywhere )
	UStaticMeshComponent* PickupMesh;

	UPROPERTY( VisibleAnywhere )
	UNiagaraComponent* PickupEffectComponent;

	UPROPERTY( EditAnywhere )
	UNiagaraSystem* PickupEffect;

	/* 
	* Delay for when we are allowed to detect overlap events 
	* This will fix the issue when object is spawned and player is already overlapping.
	* This will prevent the APickupSpawnPoint from binding the OnDestroyed delegate because the actor
	* will already be destroyed.
	*/
	FTimerHandle BindOverlapTimer;
	float BindOverlapTime = 0.25f;
	void BindOverlapTimerFinished();

};
