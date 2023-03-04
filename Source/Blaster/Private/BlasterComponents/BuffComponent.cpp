// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterCharacter.h"

// Sets default values for this component's properties
UBuffComponent::UBuffComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UBuffComponent::BeginPlay()
{
	Super::BeginPlay();

}


// Called every frame
void UBuffComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	HealRampUp( DeltaTime );
}

void UBuffComponent::Heal( float HealAmount, float HealingTime )
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp( float DeltaTime )
{
	if (!bHealing || !Character || Character->IsEliminated()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth( FMath::Clamp( Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth() ) );
	Character->UpdateHUDHealth();
	AmmountToHeal -= HealThisFrame;

	if (AmmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmmountToHeal = 0;
	}
}

