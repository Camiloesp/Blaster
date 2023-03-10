// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"


// Stores box info
USTRUCT( BlueprintType )
struct FBoxInformation
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FVector Location;
	UPROPERTY()
	FRotator Rotation;
	UPROPERTY()
	FVector BoxExtent;

};


// package all BoxInformation into packages with its time.
USTRUCT( BlueprintType )
struct FFramePackage
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ABlasterCharacter* Character;
};

USTRUCT( BlueprintType )
struct FServerSideRewindResult
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bHitConfirmed;
	UPROPERTY()
	bool bHeadshot;
};

USTRUCT( BlueprintType )
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> HeadShots;
	UPROPERTY()
	TMap<ABlasterCharacter*, uint32> BodyShots;

};

class ABlasterCharacter;
class ABlasterPlayerController;
class AWeapon;


UCLASS( ClassGroup = (Custom), meta = (BlueprintSpawnableComponent) )
class BLASTER_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULagCompensationComponent();
	friend ABlasterCharacter;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;


	/* ULagCompensationComponent */

public:

	void ShowFramePackage( const FFramePackage& Package, const FColor Color );

	/* HitScan */
	FServerSideRewindResult ServerSideRewind( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime );
	
	/* Projectile */
	FServerSideRewindResult ProjectileServerSideRewind( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime );

	/* Shotgun */
	FShotgunServerSideRewindResult ShotgunServerSideRewind( const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime );

	UFUNCTION(Server, Reliable)
	void ServerScoreRequest( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime, AWeapon* DamageCauser );
	UFUNCTION( Server, Reliable )
	void ProjectileServerScoreRequest( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime );
	UFUNCTION( Server, Reliable )
	void ShotgunServerScoreRequest( const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime );

protected:

	void SaveFramePackage( FFramePackage& Package );
	FFramePackage InterpBetweenFrames( const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime );
	void CacheBoxPositions( ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage );
	void MoveBoxes( ABlasterCharacter* HitCharacter, const FFramePackage& Package );
	void ResetHitBoxes( ABlasterCharacter* HitCharacter, const FFramePackage& Package );
	void EnableCharacterMeshCollision( ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled );
	void SaveFramePackage();
	FFramePackage GetFrameToCheck( ABlasterCharacter* HitCharacter, float HitTime );

	/* HitScan */
	FServerSideRewindResult ConfirmHit( const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation );

	/* Projectile */
	FServerSideRewindResult ProjectileConfirmHit( const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime );

	/* 
	* Shotgun 
	*/
	FShotgunServerSideRewindResult ShotgunConfirmHit( const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations );

private:

	UPROPERTY()
	ABlasterCharacter* Character;
	UPROPERTY()
	ABlasterPlayerController* Controller;

	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
