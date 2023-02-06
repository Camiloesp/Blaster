// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/BlasterAnimInstance.h"
#include "Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UBlasterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
}

void UBlasterAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (!BlasterCharacter) BlasterCharacter = Cast<ABlasterCharacter>(TryGetPawnOwner());
	if (!BlasterCharacter) return;

	// Get speed
	FVector Velocity = BlasterCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	// Get bIsInAir
	bIsInAir = BlasterCharacter->GetCharacterMovement()->IsFalling();
	// Get bIsAccelerating by getting Acceleration.Size(). If it is > 0 then it will be true
	bIsAccelerating = BlasterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0 ? true : false;

	bWeaponEquipped = BlasterCharacter->IsWeaponEquipped();

	bIsCrouched = BlasterCharacter->bIsCrouched;

	bAiming = BlasterCharacter->IsAiming();

	// Offset Yaw for strafing
	FRotator AimRotation = BlasterCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(BlasterCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 6.f); // Prevents weird interpolation from -180 to 0 to 180. Instead  goes to the closest from -180 to 180 without going around
	YawOffset = DeltaRotation.Yaw;

	// Leaning
	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = BlasterCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame); // Small value, we have to make it bigger, and ensure its scaled in some way with delta time, that way we have framerate independency
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 6.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);
}
