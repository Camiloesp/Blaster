// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerController/BlasterPlayerController.h"
#include "HUD/BlasterHUD.h"
#include "HUD/CharacterOverlay.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Character/BlasterCharacter.h"
#include "GameModes/BlasterGameMode.h"
#include "HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "BlasterComponents/CombatComponent.h"
#include "GameState/BlasterGameState.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Components/Image.h"
#include "Weapon/Weapon.h"
#include "HUD/ReturnToMainMenu.h"
#include "BlasterTypes/Announcement.h"
#include "Net/UnrealNetwork.h"

// Enhanced input
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "InputActions/BlasterInputConfigData.h" // list of inputs


void ABlasterPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ABlasterPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ABlasterPlayerController, MatchState);
	DOREPLIFETIME( ABlasterPlayerController, bShowTeamScores );
}

void ABlasterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
	ServerCheckMatchState();
}

void ABlasterPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
}

void ABlasterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!InputComponent) return;
	
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>( InputComponent ))
	{
		EnhancedInputComponent->BindAction( QuitButton, ETriggerEvent::Triggered, this, &ABlasterPlayerController::ShowReturnToMainMenu );
	}
}

void ABlasterPlayerController::ShowReturnToMainMenu( const FInputActionValue& Value )
{
	if (!ReturnToMainMenuWidget) return;
	if (!ReturnToMainMenu)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMenu>( this, ReturnToMainMenuWidget );
	}

	if (ReturnToMainMenu)
	{
		bReturnToMainMenuOpen = !bReturnToMainMenuOpen;
		if (bReturnToMainMenuOpen)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

void ABlasterPlayerController::CheckPing( float DeltaTime )
{
	// Check ping
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = !PlayerState ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			//GEngine->AddOnScreenDebugMessage( 1, 4.f, FColor::Red, FString::Printf(TEXT("Ping: %d"), PlayerState->GetCompressedPing() * 4 ));

			// Compressed ping. This function returns ping/4.  That is why we get the ping ourselves in CheckTimeSync(). For a high ping warning this is okay
			if (PlayerState->GetCompressedPing() * 4 > HighPingThreshold) // Warning: Dont use GetPing(). use GetCompressedPing or GetPingInMilliseconds
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus( true );
			}
			else
			{
				ServerReportPingStatus( false );
			}
		}
		HighPingRunningTime = 0.f;
	}

	bool bHighPingAnimationPlaying =
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingAnimation &&
		BlasterHUD->CharacterOverlay->IsAnimationPlaying( BlasterHUD->CharacterOverlay->HighPingAnimation
		);

	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

// Is the ping too high?
void ABlasterPlayerController::ServerReportPingStatus_Implementation( bool bHighPing )
{
	HighPingDelegate.Broadcast( bHighPing );
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController())
	{
		if (TimeSyncRunningTime > TimeSyncFrequency)
		{
			ServerRequestServerTime(GetWorld()->GetTimeSeconds());
			TimeSyncRunningTime = 0;
		}
	}
}

void ABlasterPlayerController::HighPingWarning()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation
		);

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f); // Should be HUD responsability.
		BlasterHUD->CharacterOverlay->PlayAnimation( BlasterHUD->CharacterOverlay->HighPingAnimation, 0.f, 5 );
	}
}

void ABlasterPlayerController::StopHighPingWarning()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HighPingImage &&
		BlasterHUD->CharacterOverlay->HighPingAnimation
		);

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->HighPingImage->SetOpacity( 0.f ); // Should be HUD responsability.
		if (BlasterHUD->CharacterOverlay->IsAnimationPlaying( BlasterHUD->CharacterOverlay->HighPingAnimation ))
		{
			BlasterHUD->CharacterOverlay->StopAnimation( BlasterHUD->CharacterOverlay->HighPingAnimation );
		}
	}
}

void ABlasterPlayerController::ServerCheckMatchState_Implementation()
{
	ABlasterGameMode* GameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidgame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		/*
		if (BlasterHUD && MatchState == MatchState::WaitingToStart)
		{
			BlasterHUD->AddAnnouncement();
		}*/			// TEST WITHOUT ! IN bLASTERhud
	}
}

void ABlasterPlayerController::ClientJoinMidgame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = Match;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);

	if (BlasterHUD && MatchState == MatchState::WaitingToStart)
	{
		BlasterHUD->AddAnnouncement();
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if (BlasterCharacter)
	{
		// personal fix. not included in tutorial the Ammo calls
		if (BlasterCharacter->GetCombat() && BlasterCharacter->GetCombat()->GetEquippedWeapon())
		{
			SetHUDCarriedAmmo( BlasterCharacter->GetCombat()->GetCarriedAmmo() );
			SetHUDWeaponAmmo( BlasterCharacter->GetCombat()->GetEquippedWeapon()->GetAmmo() );
		}

		ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if (BlasterPlayerState) // Called on Server.
		{
			FString PlayerName = BlasterPlayerState->GetPlayerName();
			BlasterCharacter->MulticastSetPlayerName( PlayerName ); // multicast
		}

		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
		SetHUDShield(BlasterCharacter->GetShield(), BlasterCharacter->GetMaxShield());
	}
}

void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText
	);

	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ABlasterPlayerController::SetHUDShield( float Shield, float MaxShield )
{
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ShieldBar &&
		BlasterHUD->CharacterOverlay->ShieldText
	);

	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		BlasterHUD->CharacterOverlay->ShieldBar->SetPercent( ShieldPercent );
		FString ShieldText = FString::Printf( TEXT( "%d / %d" ), FMath::CeilToInt( Shield ), FMath::CeilToInt( MaxShield ) );
		BlasterHUD->CharacterOverlay->ShieldText->SetText( FText::FromString( ShieldText ) );
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ABlasterPlayerController::SetHUDScore(float Score)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount
	);

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount
		);

	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount
		);

	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount
		);

	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText
		);

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->Announcement &&
		BlasterHUD->Announcement->WarmupTime
		);

	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			BlasterHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}

		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		BlasterHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ABlasterPlayerController::SetHUDGrenades(int32 Grenades)
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());

	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->GrenadesText
		);

	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		BlasterHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ABlasterPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if (MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		BlasterGameMode = BlasterGameMode ? BlasterGameMode : Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
		if (BlasterGameMode)
		{
			SecondsLeft = FMath::CeilToInt(BlasterGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}

	CountdownInt = SecondsLeft;
}

void ABlasterPlayerController::PollInit()	// Set HUD in tick
{
	if (!CharacterOverlay)
	{
		if (BlasterHUD && BlasterHUD->CharacterOverlay)
		{
			CharacterOverlay = BlasterHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if (bInitializeHealth)		SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield)		SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore)		SetHUDScore(HUDScore);
				if (bInitializeDefeats)		SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo)	SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo)	SetHUDWeaponAmmo(HUDWeaponAmmo);

				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
				if (BlasterCharacter && BlasterCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(BlasterCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = (0.5f * RoundTripTime);
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void ABlasterPlayerController::HideTeamScores()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->RedTeamScore &&
		BlasterHUD->CharacterOverlay->BlueTeamScore &&
		BlasterHUD->CharacterOverlay->ScoreSpacerText
	);

	if (bHUDValid)
	{
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText( FText() );
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText( FText() );
		BlasterHUD->CharacterOverlay->ScoreSpacerText->SetText( FText() );
	}
}

void ABlasterPlayerController::InitTeamScores()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->RedTeamScore &&
		BlasterHUD->CharacterOverlay->BlueTeamScore &&
		BlasterHUD->CharacterOverlay->ScoreSpacerText
		);

	if (bHUDValid)
	{
		FString Zero( "0" );
		FString Spacer( "|" );
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText( FText::FromString( Zero ) );
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText( FText::FromString( Zero ) );
		BlasterHUD->CharacterOverlay->ScoreSpacerText->SetText( FText::FromString( Spacer ) );
	}
}

void ABlasterPlayerController::SetHUDRedTeamScore( int32 RedScore )
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->RedTeamScore
		);

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf( TEXT("%d"), RedScore );
		BlasterHUD->CharacterOverlay->RedTeamScore->SetText( FText::FromString( ScoreText ) );
	}
}

void ABlasterPlayerController::SetHUDBlueTeamScore( int32 BlueScore )
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
	bool bHUDValid = (
		BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->BlueTeamScore
		);

	if (bHUDValid)
	{
		FString ScoreText = FString::Printf( TEXT( "%d" ), BlueScore );
		BlasterHUD->CharacterOverlay->BlueTeamScore->SetText( FText::FromString( ScoreText ) );
	}
}

void ABlasterPlayerController::OnRep_ShowTeamScores()
{
	if (bShowTeamScores)
	{
		InitTeamScores();
	}
	else
	{
		HideTeamScores();
	}
}

float ABlasterPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	else return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::OnMatchStateSet( FName State, bool bTeamsMatch )
{
	MatchState = State;

	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted( bTeamsMatch );
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ABlasterPlayerController::HandleMatchHasStarted( bool bTeamsMatch )
{
	if (HasAuthority()) bShowTeamScores = bTeamsMatch;
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (BlasterHUD)
	{
		BlasterHUD->AddCharacterOverlay(); // If being added twice, add check : if(!BlasterHUD->CharacterOverlay)
		if (BlasterHUD->Announcement)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}

		if (!HasAuthority()) return;
		if (bTeamsMatch)
		{
			InitTeamScores();
		}
		else
		{
			HideTeamScores();
		}
	}
}

void ABlasterPlayerController::HandleCooldown()
{
	BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>(GetHUD());
	if (BlasterHUD)
	{
		BlasterHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = (BlasterHUD->Announcement &&
			BlasterHUD->Announcement->AnnouncementText &&
			BlasterHUD->Announcement->InfoText);
		if (bHUDValid)
		{
			BlasterHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText = Announcement::NewMatchStartsIn;
			BlasterHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this));
			ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
			if (BlasterGameState && BlasterPlayerState)
			{
				TArray<ABlasterPlayerState*> TopPlayers = BlasterGameState->TopScoringPlayers;
				FString InfoTextString = bShowTeamScores ? GetTeamsInfoText( BlasterGameState ) : GetInfoText( TopPlayers );

				BlasterHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}

	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(GetPawn());
	if (BlasterCharacter && BlasterCharacter->GetCombat())
	{
		BlasterCharacter->bDisableGameplay = true;
		BlasterCharacter->GetCombat()->FireButtonPressed(false);
	}
}

FString ABlasterPlayerController::GetInfoText( const TArray<ABlasterPlayerState*>& Players )
{
	ABlasterPlayerState* BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
	if (!BlasterPlayerState) return FString();
	FString InfoTextString;
	if (Players.Num() == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (Players.Num() == 1 && Players[0] == BlasterPlayerState)	// Only one player
	{
		InfoTextString = Announcement::YouAreTheWinner;
	}
	else if (Players.Num() == 1) // Another player won, not this one.
	{
		InfoTextString = FString::Printf( TEXT( "Winner: \n%s" ), *Players[0]->GetPlayerName() );
	}
	else if (Players.Num() > 1)
	{
		InfoTextString = Announcement::PlayersTiedForTheWin;
		InfoTextString.Append( FString( "\n" ) );
		for (auto TiedPlayer : Players)
		{
			InfoTextString.Append( FString::Printf( TEXT( "%s\n" ), *TiedPlayer->GetPlayerName() ) );
		}
	}
	return InfoTextString;
}

FString ABlasterPlayerController::GetTeamsInfoText( ABlasterGameState* BlasterGameState )
{
	if (!BlasterGameState) return FString();
	FString InfoTextString;

	const int32 RedTeamScore = BlasterGameState->RedTeamScore;
	const int32 BlueTeamScore = BlasterGameState->BlueTeamScore;

	if (RedTeamScore == 0 && BlueTeamScore == 0)
	{
		InfoTextString = Announcement::ThereIsNoWinner;
	}
	else if (RedTeamScore == BlueTeamScore)
	{
		InfoTextString = FString::Printf( TEXT("%s\n"), *Announcement::TeamsTiedForTheWin );
		InfoTextString.Append( Announcement::RedTeam );
		InfoTextString.Append( TEXT("\n"));
		InfoTextString.Append( Announcement::BlueTeam );
		InfoTextString.Append( TEXT( "\n" ) );
	}
	else if (RedTeamScore > BlueTeamScore)
	{
		InfoTextString = Announcement::RedTeamWins;
		InfoTextString.Append( TEXT( "\n" ) );
		InfoTextString.Append( FString::Printf( TEXT("%s: %d"), *Announcement::RedTeam, RedTeamScore ));
		InfoTextString.Append( TEXT( "\n" ) );
		InfoTextString.Append( FString::Printf( TEXT( "%s: %d" ), *Announcement::BlueTeam, BlueTeamScore ) );
	}
	else if (RedTeamScore < BlueTeamScore)
	{
		InfoTextString = Announcement::BlueTeamWins;
		InfoTextString.Append( TEXT( "\n" ) );
		InfoTextString.Append( FString::Printf( TEXT( "%s: %d" ), *Announcement::BlueTeam, BlueTeamScore ) );
		InfoTextString.Append( TEXT( "\n" ) );
		InfoTextString.Append( FString::Printf( TEXT( "%s: %d" ), *Announcement::RedTeam, RedTeamScore ) );
	}

	return InfoTextString;
}

void ABlasterPlayerController::BroadcastElimination( APlayerState* Attacker, APlayerState* Victim )
{
	ClientEliminationAnnouncement( Attacker, Victim );
}

void ABlasterPlayerController::ClientEliminationAnnouncement_Implementation( APlayerState* Attacker, APlayerState* Victim )
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		BlasterHUD = BlasterHUD ? BlasterHUD : Cast<ABlasterHUD>( GetHUD() );
		if (BlasterHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				BlasterHUD->AddEliminationAnnouncement( "You", Victim->GetPlayerName(), Attacker, Victim );
				return;
			}
			if (Victim == Self && Attacker != Self)
			{
				BlasterHUD->AddEliminationAnnouncement( Attacker->GetPlayerName(), "You", Attacker, Victim );
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				BlasterHUD->AddEliminationAnnouncement( "You", "Yourself", Attacker, Victim );
				return;
			}
			if (Attacker==Victim && Attacker != Self)
			{
				BlasterHUD->AddEliminationAnnouncement( Attacker->GetPlayerName(), "Themselves", Attacker, Victim );
				return;
			}
			BlasterHUD->AddEliminationAnnouncement( Attacker->GetPlayerName(), Victim->GetPlayerName(), Attacker, Victim );
		}
	}
}
