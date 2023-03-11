// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ReturnToMainMenu.generated.h"

class UButton;
class APlayerController;
class UMultiplayerSessionsSubsystem;
/**
 *
 */
UCLASS()
class BLASTER_API UReturnToMainMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override; // BeginPlay. delegate binding too early here

	/* UReturnToMainMenu */
public:
	void MenuSetup();
	void MenuTearDown();

protected:

	UFUNCTION()
	void OnDestroySession( bool bWasSuccessful );
	UFUNCTION()
	void OnPlayerLeftGame();

private:

	UPROPERTY( meta = (BindWidget) )
	UButton* ReturnButton;

	UFUNCTION()
	void ReturnButtonClicked();

	UPROPERTY()
	UMultiplayerSessionsSubsystem* MultiplayerSessionsSubsystem;

	UPROPERTY()
	APlayerController* PlayerController;

};
