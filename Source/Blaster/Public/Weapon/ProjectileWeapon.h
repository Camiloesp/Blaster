// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "ProjectileWeapon.generated.h"

class AProjectile;
/**
 * 
 */
UCLASS()
class BLASTER_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()
	
private:

	UPROPERTY(EditAnywhere)
	TSubclassOf<AProjectile> ProjectileClass;

public:

	virtual void Fire(const FVector& HitTarget) override;
};
