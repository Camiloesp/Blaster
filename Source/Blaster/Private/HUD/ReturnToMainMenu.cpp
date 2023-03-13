// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/ReturnToMainMenu.h"
#include "GameframeWork/PlayerController.h"
#include "Components/Button.h"
#include "GameFramework/GameModeBase.h"
#include "Character/BlasterCharacter.h"

#include "MultiplayerSessionsSubsystem.h" // To access plugin classes we need to add them to our project .build

bool UReturnToMainMenu::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	return true;
}

void UReturnToMainMenu::MenuSetup()
{
	AddToViewport();
	SetVisibility( ESlateVisibility::Visible );
	bIsFocusable = true;

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = !PlayerController ? World->GetFirstPlayerController() : PlayerController; // or GetOwningPlayer()
		if (PlayerController)
		{
			FInputModeGameAndUI InputModeData;
			InputModeData.SetWidgetToFocus( TakeWidget() );
			PlayerController->SetInputMode( InputModeData );
			PlayerController->SetShowMouseCursor( true );
		}
	}

	if (ReturnButton && !ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.AddDynamic( this, &UReturnToMainMenu::ReturnButtonClicked );
	}
	UGameInstance* GameInstance = GetGameInstance();
	if (GameInstance)
	{
		MultiplayerSessionsSubsystem = GameInstance->GetSubsystem<UMultiplayerSessionsSubsystem>();
		if (MultiplayerSessionsSubsystem)// && !MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
		{
			MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.AddDynamic( this, &UReturnToMainMenu::OnDestroySession );
		}
	}
}

void UReturnToMainMenu::MenuTearDown()
{
	RemoveFromParent();

	UWorld* World = GetWorld();
	if (World)
	{
		PlayerController = !PlayerController ? World->GetFirstPlayerController() : PlayerController;
		if (PlayerController)
		{
			FInputModeGameOnly InputModeData;
			PlayerController->SetInputMode( InputModeData );
			PlayerController->SetShowMouseCursor( false );
		}
	}

	if (ReturnButton && ReturnButton->OnClicked.IsBound())
	{
		ReturnButton->OnClicked.RemoveDynamic( this, &UReturnToMainMenu::ReturnButtonClicked );
	}
	if (MultiplayerSessionsSubsystem && MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.IsBound())
	{
		MultiplayerSessionsSubsystem->MultiplayerOnDestroySessionComplete.RemoveDynamic( this, &UReturnToMainMenu::OnDestroySession );
	}
}

void UReturnToMainMenu::OnDestroySession( bool bWasSuccessful )
{
	if (!bWasSuccessful)
	{
		ReturnButton->SetIsEnabled( true );
		return;
	}

	UWorld* World = GetWorld();
	if (World)
	{
		AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>();
		if (GameMode) // server
		{
			GameMode->ReturnToMainMenuHost();
		}
		else //client
		{
			PlayerController = !PlayerController ? World->GetFirstPlayerController() : PlayerController;
			if (PlayerController)
			{
				PlayerController->ClientReturnToMainMenuWithTextReason( FText() ); // better use this than ClientReturnToMainMenu() to avoid UE warning that tells us this one is better.
			}
		}
	}
}

void UReturnToMainMenu::ReturnButtonClicked()
{
	ReturnButton->SetIsEnabled( false );

	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* FirstPlayerController = World->GetFirstPlayerController();
		if (FirstPlayerController)
		{
			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>( FirstPlayerController->GetPawn() );
			if (BlasterCharacter)
			{
				BlasterCharacter->ServerLeaveGame();
				BlasterCharacter->OnLeftGame.AddDynamic( this, &UReturnToMainMenu::OnPlayerLeftGame );
			}
			else
			{
				ReturnButton->SetIsEnabled( true );
			}
		}
	}
}

void UReturnToMainMenu::OnPlayerLeftGame()
{
	// Leave online session.
	if (MultiplayerSessionsSubsystem)
	{
		MultiplayerSessionsSubsystem->DestroySession(); // calls OnDestroySession
	}
}
