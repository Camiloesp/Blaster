// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BlasterPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FHighPingDelegate, bool, bPingTooHigh );

class UUserWidget;
class ABlasterHUD;
class UReturnToMainMenu;
class UCharacterOverlay;
class ABlasterGameMode;
class UInputAction;
struct FInputActionValue;
/**
 *
 */
UCLASS()
class BLASTER_API ABlasterPlayerController : public APlayerController
{
	GENERATED_BODY()


public:

	virtual void OnPossess( APawn* InPawn ) override;
	virtual void ReceivedPlayer() override;	// Sync with server clock as soon as possible
	virtual void GetLifetimeReplicatedProps( TArray<FLifetimeProperty>& OutLifetimeProps ) const override;


protected:

	virtual void BeginPlay() override;
	virtual void Tick( float DeltaTime ) override;

	virtual void SetupInputComponent() override;

	/* ABlasterPlayerController */

public:

	void SetHUDHealth( float Health, float MaxHealth );
	void SetHUDShield( float Shield, float MaxShield );
	void SetHUDScore( float Score );
	void SetHUDDefeats( int32 Defeats );
	void SetHUDWeaponAmmo( int32 Ammo );
	void SetHUDCarriedAmmo( int32 Ammo );
	void SetHUDMatchCountdown( float CountdownTime );
	void SetHUDAnnouncementCountdown( float CountdownTime );
	void SetHUDGrenades( int32 Grenades );

	virtual float GetServerTime(); // Synced with server world clock
	void OnMatchStateSet( FName State );
	void HandleMatchHasStarted();
	void HandleCooldown();

	void BroadcastElimination( APlayerState* Attacker, APlayerState* Victim );

	float SingleTripTime = 0.f;

	FHighPingDelegate HighPingDelegate;

protected:

	UFUNCTION(Client, Reliable)
	void ClientEliminationAnnouncement( APlayerState* Attacker, APlayerState* Victim );

	void ShowReturnToMainMenu( const FInputActionValue& Value );
	/*
	* Return to main menu
	*/
	UPROPERTY(EditAnywhere, Category = HUD)
	TSubclassOf<UUserWidget> ReturnToMainMenuWidget;
	UPROPERTY()
	UReturnToMainMenu* ReturnToMainMenu;
	bool bReturnToMainMenuOpen = false;


	void SetHUDTime();
	void PollInit();	// Create a FlyingPawn for this instead?
	/*
	* Sync time between client and server
	*/

	// Requests the current server time, passing in the client's time when the request was sent
	UFUNCTION( Server, Reliable )
	void ServerRequestServerTime( float TimeOfClientRequest );
	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION( Client, Reliable )
	void ClientReportServerTime( float TimeOfClientRequest, float TimeServerReceivedClientRequest );

	float ClientServerDelta = 0.f; // Difference between client and server time.

	UPROPERTY( EditAnywhere, Category = Time )
	float TimeSyncFrequency = 5.f;
	float TimeSyncRunningTime = 0.f;
	void CheckTimeSync( float DeltaTime );

	UFUNCTION( Server, Reliable )
	void ServerCheckMatchState();
	UFUNCTION( Client, Reliable )
	void ClientJoinMidgame( FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime );

	void HighPingWarning();
	void StopHighPingWarning();
	void CheckPing(float DeltaTime);

private:

	UPROPERTY( EditAnywhere )
	UInputAction* QuitButton;

	UPROPERTY()
	ABlasterHUD* BlasterHUD;

	UPROPERTY()
	ABlasterGameMode* BlasterGameMode;

	float LevelStartingTime = 0.f;
	float MatchTime = 0.f;
	float WarmupTime = 0.f;
	float CooldownTime = 0.f;
	uint32 CountdownInt = 0;

	UPROPERTY( ReplicatedUsing = OnRep_MatchState )
	FName MatchState;
	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	UCharacterOverlay* CharacterOverlay;

	float HUDHealth;
	bool bInitializeHealth = false;
	float HUDMaxHealth;
	float HUDScore;
	bool bInitializeScore = false;
	int32 HUDDefeats;
	bool bInitializeDefeats = false;
	int32 HUDGrenades;
	bool bInitializeGrenades = false;
	float HUDShield;
	bool bInitializeShield = false;
	float HUDMaxShield;

	float HUDCarriedAmmo;
	bool bInitializeCarriedAmmo = false;
	float HUDWeaponAmmo;
	bool bInitializeWeaponAmmo = false; //bHUDWeaponAmmo


	/* Ping */
	float HighPingRunningTime = 0.f;
	UPROPERTY(EditAnywhere)
	float HighPingDuration = 5.f;
	float PingAnimationRunningTime = 0.f;
	UPROPERTY( EditAnywhere )
	float CheckPingFrequency = 20.f;
	UFUNCTION(Server, Reliable)
	void ServerReportPingStatus( bool bHighPing );
	// Ping higher than 50 (Miliseconds lecture 208 at end) seconds is where the player starts noticing performance hits due to network connection.
	UPROPERTY( EditAnywhere )
	float HighPingThreshold = 50.f;
};
