// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABlasterCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UBuffComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBuffComponent();
	friend ABlasterCharacter;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;


	/* UBuffComponent */
public:

	void Heal(float HealAmount, float HealingTime);

protected:

	void HealRampUp(float DeltaTime);

private:

	UPROPERTY()
	ABlasterCharacter* Character;

	bool bHealing = false;
	float HealingRate = 0.f;
	float AmmountToHeal = 0.f;

};
