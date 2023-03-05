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
	void BuffSpeed( float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime );
	void BuffJump( float BuffJumpVelocity, float BuffTime );

	void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed);
	void SetInitialJumpVelocity(float Velocity);


protected:

	void HealRampUp(float DeltaTime);

private:

	UPROPERTY()
	ABlasterCharacter* Character;

	/* Healing */
	bool bHealing = false;
	UPROPERTY(EditAnywhere)
	float HealingRate = 0.f;
	UPROPERTY( EditAnywhere )
	float AmmountToHeal = 0.f;

	/* Speed */
	FTimerHandle SpeedBuffTimer;
	void ResetSpeeds();
	float InitialBaseSpeed;
	float InitialCrouchSpeed;
	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed);

	/* Jump */
	FTimerHandle JumpBuffTimer;
	void ResetJump();
	float InitialJumpVelocity;
	UFUNCTION( NetMulticast, Reliable )
	void MulticastJumpBuff(float JumpVelocity);

};
