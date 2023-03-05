// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "BlasterComponents/CombatComponent.h"

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

void UBuffComponent::SetInitialSpeeds( float BaseSpeed, float CrouchSpeed )
{
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::SetInitialJumpVelocity( float Velocity )
{
	InitialJumpVelocity = Velocity;
}

// Called every frame
void UBuffComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	HealRampUp( DeltaTime );
	ShieldRampUp( DeltaTime );
}

void UBuffComponent::Heal( float HealAmount, float HealingTime )
{
	bHealing = true;
	HealingRate = HealAmount / HealingTime;
	AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp( float DeltaTime )
{
	if (!bHealing || !Character || Character->IsEliminated()) return;

	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth( FMath::Clamp( Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth() ) );
	Character->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;

	if (AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth())
	{
		bHealing = false;
		AmountToHeal = 0;
	}
}

void UBuffComponent::ReplenishShield( float ShieldAmount, float ReplenishTime )
{
	bReplenishingShield = true;
	ShieldReplenishRate = ShieldAmount / ReplenishTime;
	ShieldReplenishAmount += ShieldAmount;
}

void UBuffComponent::ShieldRampUp( float DeltaTime )
{
	if (!bReplenishingShield || !Character || Character->IsEliminated()) return;

	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime;
	Character->SetShield( FMath::Clamp( Character->GetShield() + ReplenishThisFrame, 0.f, Character->GetMaxShield() ) );
	Character->UpdateHUDShield();
	ShieldReplenishAmount -= ReplenishThisFrame;

	if (ShieldReplenishAmount <= 0.f || Character->GetShield() >= Character->GetMaxShield())
	{
		bReplenishingShield = false;
		ShieldReplenishAmount = 0;
	}
}

void UBuffComponent::BuffSpeed( float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime )
{
	if (!Character) return;

	Character->GetWorldTimerManager().SetTimer( SpeedBuffTimer, this, &UBuffComponent::ResetSpeeds, BuffTime );

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	MulticastSpeedBuff( BuffBaseSpeed, BuffCrouchSpeed );
}

void UBuffComponent::ResetSpeeds()
{
	if (!Character || !Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;

	MulticastSpeedBuff( InitialBaseSpeed, InitialCrouchSpeed );
}

void UBuffComponent::MulticastSpeedBuff_Implementation( float BaseSpeed, float CrouchSpeed )
{
	if (!Character || !Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = BaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;

	if (Character->GetCombat())
	{
		Character->GetCombat()->SetSpeeds( BaseSpeed, CrouchSpeed );
	}
}

void UBuffComponent::BuffJump( float BuffJumpVelocity, float BuffTime )
{
	if (!Character) return;

	Character->GetWorldTimerManager().SetTimer( JumpBuffTimer, this, &UBuffComponent::ResetJump, BuffTime );

	if (Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->JumpZVelocity = BuffJumpVelocity;
	}
	MulticastJumpBuff( BuffJumpVelocity );
}

void UBuffComponent::ResetJump()
{
	if (!Character || !Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->JumpZVelocity = InitialJumpVelocity;
	MulticastJumpBuff( InitialJumpVelocity );
}

void UBuffComponent::MulticastJumpBuff_Implementation( float JumpVelocity )
{
	if (!Character || !Character->GetCharacterMovement()) return;

	Character->GetCharacterMovement()->JumpZVelocity = JumpVelocity;
}
