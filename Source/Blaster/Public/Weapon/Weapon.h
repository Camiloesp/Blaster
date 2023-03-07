// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Weapon.generated.h"

class USphereComponent;
class UWidgetComponent;
class UAnimationAsset;
class ACasing;
class ABlasterCharacter;
class ABlasterPlayerController;
class USoundCue;

UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial				UMETA(DisplayName = "Initial State"),			// State for when the weapon is sitting in the world that has never been picked up yet.
	EWS_Equipped			UMETA(DisplayName = "Equipped"),				// Weapon being used.
	EWS_EquippedSecondary	UMETA( DisplayName = "Equipped Secondary" ),	// Weapon being used.
	EWS_Dropped				UMETA(DisplayName = "Dropped"),					// Dropping weapon. It bounces with physics.

	EWS_Max					UMETA(DisplayName = "DefaultMAX")				// How many enum constants are in EWeaponState
};

UENUM( BlueprintType )
enum class EFireType : uint8
{
	EFT_HitScan		UMETA( DisplayName = "Hit Scan Weapon" ),
	EFT_Projectile	UMETA( DisplayName = "Projectile Weapon" ),
	EFT_Shotgun		UMETA( DisplayName = "Shotgun" ),

	EWS_Max			UMETA( DisplayName = "DefaultMAX" )
};

class UTexture2D;

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWeapon();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Owner() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	/* AWeapon */

private:

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	/* Collision for pickup */
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UWidgetComponent* PickupWidget;

	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	UFUNCTION()
	void OnRep_WeaponState();

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACasing> CasingClass;

	/*
	* Ammo variables
	*/
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);
	UFUNCTION( Client, Reliable )
	void ClientAddAmmo(int32 AmmoToAdd);
	void SpendRound();

	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	// The number of unprocessed server requests for Ammo
	// Incremented in SpendRound, decremented in ClientUpdateAmmo
	int32 Sequence = 0;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	

protected:
	
	virtual void OnWeaponStateSet();
	virtual void OnEquipped();
	virtual void OnDropped();
	virtual void OnEquippedSecondary();

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);


	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UPROPERTY( EditAnywhere )
	float Damage = 20.f;

	UPROPERTY( EditAnywhere )
	bool bUseServerSideRewind = false;

	UPROPERTY()
	ABlasterCharacter* BlasterOwnerCharacter;
	UPROPERTY()
	ABlasterPlayerController* BlasterOwnerController;
public:

	/*
	* Textures for the weapon crosshairs
	*/

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;

	/*
	* Zoomed FOV while aiming
	*/
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;


	/* 
	* Automatic Fire
	*/
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	bool bDestroyWeapon = false;

	UPROPERTY( EditAnywhere )
	EFireType FireType;

	/*
	* Trace end with scatter.
	*/
	UPROPERTY( EditAnywhere, Category = "Weapon Scatter" )
	float DistanceToSphere = 800.f;
	UPROPERTY( EditAnywhere, Category = "Weapon Scatter" )
	float SphereRadius = 75.f;
	UPROPERTY( EditAnywhere, Category = "Weapon Scatter" )
	bool bUseScatter = false;

	UPROPERTY(EditAnywhere)
	USoundCue* EquipSound;

	void ShowPickupWidget(bool bShowWidget);

	virtual void Fire(const FVector& HitTarget);
	void Dropped();

	void AddAmmo(int32 AmmoToAdd);

	FVector TraceEndWithScatter( const FVector& HitTarget );

	void SetHUDAmmo();

	/*
	* Enable/Disable custom depth. (Outline color for weapons)
	*/
	void EnableCustomDepth(bool bEnable);


	/* getters */
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }
	FORCEINLINE float GetDamage() const { return Damage; }
	bool IsEmpty();
	bool IsFull();

	/* setters */
	void SetWeaponState(EWeaponState NewWeaponState);
};
