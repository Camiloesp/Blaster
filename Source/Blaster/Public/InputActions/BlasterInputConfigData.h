// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EnhancedInput/Public/InputAction.h"
#include "BlasterInputConfigData.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UBlasterInputConfigData : public UDataAsset
{
	GENERATED_BODY()
	

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* EquipAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* AimAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* FireAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* ReloadAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* ThrowGrenade;

};
