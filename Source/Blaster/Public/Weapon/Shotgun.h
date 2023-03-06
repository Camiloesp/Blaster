// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()
	


		/* AShotgun */

public:

	virtual void FireShotgun( const TArray<FVector_NetQuantize>& HitTargets );

	void ShotgunTraceEndWithScatter( const FVector& HitTarget, TArray<FVector_NetQuantize>& HitTargets );

private:

	// Number of metal bits inside the shotgun shell.
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	uint32 NumberOfPellets = 10;
};
