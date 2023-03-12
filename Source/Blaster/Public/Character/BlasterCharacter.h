// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActions/BlasterInputConfigData.h"
#include "BlasterTypes/TurningInPlace.h"
#include "Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "BlasterTypes/CombatState.h"
#include "BlasterCharacter.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE( FOnLeftGame );


class USpringArmComponent;
class UCameraComponent;
class UWidgetComponent;
class UInputMappingContext;
class UAnimMontage;
class USoundCue;
class UBoxComponent;
class ABlasterPlayerState;
class AWeapon;
class UCombatComponent;
class UBuffComponent;
class ULagCompensationComponent;
class ABlasterPlayerController;
class UNiagaraSystem;
class UNiagaraComponent;

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
	virtual void Jump() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetupInputMappingContext();

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

	/*
	* Blaster Components
	*/

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	UCombatComponent* Combat;

	UPROPERTY( VisibleAnywhere)
	UBuffComponent* Buff;
	UPROPERTY( VisibleAnywhere )
	ULagCompensationComponent* LagCompensation;

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

	/*
	* Animation montages
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* EliminatedMontage;
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ThrowGrenadeMontage;
	UPROPERTY( EditAnywhere, Category = Combat )
	UAnimMontage* SwapMontage; // SwapWeapon


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
	void OnRep_Health(float LastHealth);

	/*
	* Player Shield
	*/
	UPROPERTY( EditAnywhere, Category = "Player Stats" )
	float MaxShield = 100.f;
	UPROPERTY( ReplicatedUsing = OnRep_Shield, EditAnywhere, Category = "Player Stats" )
	float Shield = 0.f;
	UFUNCTION()
	void OnRep_Shield(float LastShield);


	ABlasterPlayerController* BlasterPlayerController;

	/*
	* Elimination
	*/
	bool bEliminated = false;
	FTimerHandle EliminatedTimer;
	UPROPERTY(EditDefaultsOnly)
	float EliminationDelay = 3.f;
	void EliminatedTimerFinished();

	bool bLeftGame = false;

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
	* Elimination FX
	*/
	UPROPERTY(EditAnywhere)
	UParticleSystem* EliminationBotEffect;
	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* EliminationBotComponent;
	UPROPERTY(EditAnywhere)
	USoundCue* EliminationBotSound;

	/*
	* Crown FX
	*/
	UPROPERTY(EditAnywhere)
	UNiagaraSystem* CrownSystem;
	UPROPERTY( EditAnywhere )
	UNiagaraComponent* CrownComponent;

	UPROPERTY()
	ABlasterPlayerState* BlasterPlayerState;

	/* Fix this bInputsSet ? */
	bool bInputsSet = false;

	/*
	* Grenade
	*/
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* AttachedGrenade;

	/*
	* Default Weapon
	*/
	UPROPERTY( EditAnywhere )
	TSubclassOf<AWeapon> DefaultWeaponClass;

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
	void ReloadButtonPressed(const FInputActionValue& Value);
	void AimButtonPressed(const FInputActionValue& Value);
	void FireButtonPressed(const FInputActionValue& Value);
	void FireButtonReleased(const FInputActionValue& Value);
	void GrenadeButtonPressed(const FInputActionValue& Value);
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

	// Poll for any relevant classes and initialize our hud
	void PollInit();
	void RotateInPlace(float DeltaTime);

	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

public:

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	void Eliminated(bool bPlayerLeftGame);
	// Handles player elimination.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastEliminated( bool bPlayerLeftGame );

	/* Play montages */
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayEliminatedMontage();
	void PlayThrowGrenadeMontage();
	void PlaySwapMontage();

	bool IsWeaponEquipped();
	bool IsAiming();

	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UFUNCTION( Server, Reliable )
	void ServerLeaveGame();
	FOnLeftGame OnLeftGame;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastGainTheLead();
	UFUNCTION( NetMulticast, Reliable )
	void MulticastLostTheLead();

	void SpawnDefaultWeapon();

	/* 
	* Hit boxes used for Server-Side rewind.
	* attached to bones. (We have the boxes be the same name as the bones)
	*/
	UPROPERTY(Editanywhere)
	UBoxComponent* head;
	UPROPERTY( Editanywhere )
	UBoxComponent* pelvis;
	UPROPERTY( Editanywhere )
	UBoxComponent* spine_02;
	UPROPERTY( Editanywhere )
	UBoxComponent* spine_03;
	UPROPERTY( Editanywhere )
	UBoxComponent* upperarm_l;
	UPROPERTY( Editanywhere )
	UBoxComponent* upperarm_r;
	UPROPERTY( Editanywhere )
	UBoxComponent* lowerarm_l;
	UPROPERTY( Editanywhere )
	UBoxComponent* lowerarm_r;
	UPROPERTY( Editanywhere )
	UBoxComponent* hand_l;
	UPROPERTY( Editanywhere )
	UBoxComponent* hand_r;
	UPROPERTY( Editanywhere )
	UBoxComponent* backpack;
	UPROPERTY( Editanywhere )
	UBoxComponent* blanket;
	UPROPERTY( Editanywhere )
	UBoxComponent* thigh_l;
	UPROPERTY( Editanywhere )
	UBoxComponent* thigh_r;
	UPROPERTY( Editanywhere )
	UBoxComponent* calf_l;
	UPROPERTY( Editanywhere )
	UBoxComponent* calf_r;
	UPROPERTY( Editanywhere )
	UBoxComponent* foot_l;
	UPROPERTY( Editanywhere )
	UBoxComponent* foot_r;

	UPROPERTY()
	TMap<FName, UBoxComponent*> HitCollisionBoxes;
	
	bool bFinishedSwapping = false;

	/* Getters */
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsEliminated() const { return bEliminated; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE UCombatComponent* GetCombat() const { return Combat; }
	FORCEINLINE bool GetDisabledGameplay() const { return bDisableGameplay; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE UBuffComponent* GetBuff() const { return Buff; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	FORCEINLINE ULagCompensationComponent* GetLagCompensation() const { return LagCompensation; }
	FORCEINLINE UInputMappingContext* GetInputMappingContext() const { return BlasterMappingContext; }
	FORCEINLINE UBlasterInputConfigData* GetInputConfigData() const { return InputConfigData; }
	AWeapon* GetEquippedWeapon();
	FVector GetHitTarget() const;
	ECombatState GetCombatState() const;
	bool IsLocallyReloading();

	/* Setters */
	FORCEINLINE void SetHealth( float Amount ) { Health = Amount; }
	FORCEINLINE void SetShield( float Amount ) { Shield = Amount; }
	void SetOverlappingWeapon(AWeapon* NewOverlappingWeapon);


	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);
};
