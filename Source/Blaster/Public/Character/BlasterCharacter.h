// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActions/BlasterInputConfigData.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"

#include "BlasterCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UInputMappingContext;
class UAnimMontage;
class USoundCue;

class AWeapon;
class UCombatComponent;
class ABlasterPlayerController;

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;	// This function we will have access to components. The CombatComponent will be constructed by this time
	virtual void OnRep_ReplicatedMovement() override;  
	virtual void Destroyed() override;

					/* ABlasterCharacter */

//TODO:
// - Change pointers to new UE standard.

private:

	UPROPERTY(VisibleAnywhere, Category = Camera)
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UWidgetComponent* OverheadWidget;

	UPROPERTY(VisibleAnywhere)
	UCombatComponent* Combat;

	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	AWeapon* OverlappingWeapon;

	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	// Aim offset
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;
	void TurnInPlace(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* EliminatedMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.5f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;
	float CalculateSpeed();


	/*
	* Player health
	*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;
	UFUNCTION()
	void OnRep_Health();
	ABlasterPlayerController* BlasterPlayerController;
	bool bEliminated = false;

	FTimerHandle EliminatedTimer;
	UPROPERTY(EditDefaultsOnly)
	float EliminationDelay = 3.f;
	void EliminatedTimerFinished();

	/*
	* Dissolve effect
	*/
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;
	FOnTimelineFloat DissolveTrack;
	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;

	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// Dynamic instance that we can change at runtime
	UPROPERTY(VisibleAnywhere, Category = Elimination)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;
	// Material instance set on the blueprint, used with the dynamic material instance.
	UPROPERTY(EditAnywhere, Category = Elimination)
	UMaterialInstance* DissolveMaterialInstance;

	/*
	* Elimination Bot
	*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* EliminationBotEffect;
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* EliminationBotComponent;
	UPROPERTY(EditAnywhere)
	USoundCue* EliminationBotSound;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* BlasterMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UBlasterInputConfigData* InputConfigData;

	// Input functions

	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void EquipButtonPressed(const FInputActionValue& Value);
	void CrouchButtonPressed(const FInputActionValue& Value);
	void AimButtonPressed(const FInputActionValue& Value);
	void FireButtonPressed(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	//virtual void Jump() override;

	// RPC from client to server
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed(const FInputActionValue& Value);//);

	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimProxiesTurn();

	void PlayHitReactMontage();

	// Fundtion signature from OnTakeAnyDamage from aactor.
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();

public:
	void Eliminated();
	// Handles player elimination.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated();

	void PlayFireMontage(bool bAiming);
	void PlayEliminatedMontage();

	bool IsWeaponEquipped();
	bool IsAiming();

	/* Getters */
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;

	/* Setters */
	void SetOverlappingWeapon(AWeapon* NewOverlappingWeapon);
};
