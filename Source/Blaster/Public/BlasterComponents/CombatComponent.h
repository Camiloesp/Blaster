// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatComponent.generated.h"

class AWeapon;
class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCombatComponent();
	// Both classes depend on each other. We need to access functions from each other so friend class will allow them to access them even if they are private/protected.
	friend ABlasterCharacter; //friend class ABlasterCharacter;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/*  UCombatComponent  */

private:

	ABlasterCharacter* Character;
	AWeapon* EquippedWeapon;

protected:
public:

	void EquipWeapon(AWeapon* WeaponToEquip);
};
