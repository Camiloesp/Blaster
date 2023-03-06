// Fill out your copyright notice in the Description page of Project Settings.

// UE includes
#include "Character/BlasterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"

// Enhanced input
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "InputActions/BlasterInputConfigData.h" // list of inputs
#include "Weapon/Weapon.h"
#include "BlasterComponents/CombatComponent.h"
#include "BlasterComponents/BuffComponent.h"
#include "Character/BlasterAnimInstance.h"
#include "Blaster/Blaster.h"
#include "PlayerController/BlasterPlayerController.h"
#include "GameModes/BlasterGameMode.h"
#include "PlayerState/BlasterPlayerState.h"
#include "Weapon/WeaponTypes.h"


// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // false since CameraBoom does it.

	// Character stands still independent from controller rotation. Orient towards its own movement direction.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(GetRootComponent());

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	// Stop colliding with other players cameras
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Grenade"));
	AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
	AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health); 
	DOREPLIFETIME( ABlasterCharacter, Shield );
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	SpawnDefaultWeapon(); // tutorial worked, but i have to call it onOnPosses in player controller to work.
	// Set HUD
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();

	// Damage binding
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}

	// Enhancedinput setup
	SetupInputMappingContext();

	if (AttachedGrenade)
	{
		AttachedGrenade->SetVisibility(false);
	}
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Server client does not move. the below code fixes it.
	if (!bInputsSet && HasAuthority() && Controller)
	{
		SetupInputMappingContext();
	}
	
	RotateInPlace(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void ABlasterCharacter::SetupInputMappingContext()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(BlasterMappingContext, 0);
			bInputsSet = true;
		}
	}
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{
	if (bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	// Not doing AimOffset if we are a simulatedProxy
	if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaTime);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaTime;
		if (TimeSinceLastMovementReplication > 0.25f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputConfigData->MoveAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Move);
		EnhancedInputComponent->BindAction(InputConfigData->LookAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Look);

		EnhancedInputComponent->BindAction(InputConfigData->JumpAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::Jump);
		EnhancedInputComponent->BindAction(InputConfigData->JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		EnhancedInputComponent->BindAction(InputConfigData->EquipAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::EquipButtonPressed);
		
		EnhancedInputComponent->BindAction(InputConfigData->CrouchAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::CrouchButtonPressed); // Input action with 2 triggers: Pressed and then Released.

		EnhancedInputComponent->BindAction(InputConfigData->AimAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::AimButtonPressed); // Input action with 2 triggers: Pressed and then Released.

		EnhancedInputComponent->BindAction(InputConfigData->FireAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::FireButtonPressed);

		EnhancedInputComponent->BindAction(InputConfigData->ReloadAction, ETriggerEvent::Triggered, this, &ABlasterCharacter::ReloadButtonPressed);

		EnhancedInputComponent->BindAction(InputConfigData->ThrowGrenade, ETriggerEvent::Triggered, this, &ABlasterCharacter::GrenadeButtonPressed);
	}
}

void ABlasterCharacter::Jump()
{
	if (bDisableGameplay) return;

	Super::Jump();
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (Combat)
	{
		Combat->Character = this;
	}

	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
		Buff->SetInitialJumpVelocity( GetCharacterMovement()->JumpZVelocity );
	}
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	
	SimProxiesTurn();
	
	TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::Destroyed()
{
	Super::Destroyed();

	if (EliminationBotComponent)
	{
		EliminationBotComponent->DestroyComponent();
	}

	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable movement when dead.

	// lecture 126 not moving problem for server-client
	if (Controller != nullptr)
	{
		const FVector2D MoveValue = Value.Get<FVector2D>();
		const FRotator MovementRotation(0, Controller->GetControlRotation().Yaw, 0);

		// Forward/Backward direction
		if (MoveValue.Y != 0.f)
		{
			// Get forward vector
			const FVector Direction = MovementRotation.RotateVector(FVector::ForwardVector);

			AddMovementInput(Direction, MoveValue.Y);
		}

		// Right/Left direction
		if (MoveValue.X != 0.f)
		{
			// Get right vector
			const FVector Direction = MovementRotation.RotateVector(FVector::RightVector);

			AddMovementInput(Direction, MoveValue.X);
		}
	}
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		const FVector2D LookValue = Value.Get<FVector2D>();

		if (LookValue.X != 0.f)
		{
			AddControllerYawInput(LookValue.X);
		}

		if (LookValue.Y != 0.f)
		{
			AddControllerPitchInput(LookValue.Y);
		}
	}
}

void ABlasterCharacter::EquipButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable action when dead.

	// Equip weapon on server.
	if (Combat)
	{
		// We dont want to do important stuff like equipping weapons on clients. Tell server to do it.
		ServerEquipButtonPressed( Value );
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation(const FInputActionValue& Value)
{
	//EquipButtonPressed(Value); better to call EquipButtonPressed instead of code repetition.
	if (Combat)
	{
		if (OverlappingWeapon)
		{
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else if (Combat->ShouldSwapWeapons())
		{
			Combat->SwapWeapons();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable action when dead.

	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}
void ABlasterCharacter::ReloadButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable action when dead.

	if (Combat)
	{
		Combat->Reload();
	}
}


void ABlasterCharacter::AimButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable action when dead.

	if (Combat)
	{
		//Combat->bAiming = !Combat->bAiming;
		Combat->SetAiming(!Combat->bAiming);
	}
}
void ABlasterCharacter::FireButtonPressed(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable action when dead.

	bool Input = Value.Get<bool>();
	if (Combat)
	{
		//GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Red, FString("Pressed!"));
		Combat->FireButtonPressed(Input);
	}
}
void ABlasterCharacter::FireButtonReleased(const FInputActionValue& Value)
{
	if (bDisableGameplay) return; // disable action when dead.

	if (Combat)
	{
		//GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Red, FString("Released!"));
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::GrenadeButtonPressed(const FInputActionValue& Value)
{
	if (Combat )
	{
		Combat->ThrowGrenade();
	}
}

float ABlasterCharacter::CalculateSpeed()
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ABlasterCharacter::OnRep_Health( float LastHealth )
{
	// instead refactor this with ReceiveDamage ? 
	UpdateHUDHealth();
	if (Health < LastHealth) PlayHitReactMontage();
}

void ABlasterCharacter::OnRep_Shield( float LastShield )
{
	UpdateHUDShield();
	if (Shield < LastShield) PlayHitReactMontage();
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
	if (Combat && !Combat->EquippedWeapon) return;

	float Speed = CalculateSpeed();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// Set Yaw offset for animation BP
	if (Speed == 0.f && !bIsInAir) // Standing still, not jumping.
	{
		bRotateRootBone = true;

		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation); // If looking around is inverted, reverse the order of this rotators in NormalizedDeltaRotator()
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
	}
	if (Speed > 0.f || bIsInAir) // Running, or jumping
	{
		bRotateRootBone = false;

		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch()
{
	// Set Pitch offset for animation BP
	AO_Pitch = GetBaseAimRotation().Pitch;

	// Fix compression issue when sending negative values to network since they are mapped to be [0 - 360). Video # 60
	if (AO_Pitch > 90.f && !IsLocallyControlled()) // if pitch is > than 90 we know we know that we can make a correction. Problem does not exist on character that we are locally controlling.
	{
		// Map pitch from [270, 360) to [-90, 0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn()
{
	/* This function fixes SimulatedProxy jittery */
	if (!Combat || !Combat->EquippedWeapon) return;

	bRotateRootBone = false;

	float Speed = CalculateSpeed();
	if (Speed > 0.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	//GEngine->AddOnScreenDebugMessage(1, 10.f, FColor::Red, FString::Printf(TEXT("ProxyYaw %f"), ProxyYaw));

	if (FMath::Abs(ProxyYaw) > TurnThreshold)
	{
		if (ProxyYaw > TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Right;
		}
		else if (ProxyYaw < -TurnThreshold)
		{
			TurningInPlace = ETurningInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}

	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;

	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;
		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("RocketLauncher");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Pistol"); // Pistol because its similar and no anim asset.
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("SniperRifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("GrenadeLauncher");
			break;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayEliminatedMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && EliminatedMontage)
	{
		AnimInstance->Montage_Play(EliminatedMontage);
	}
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}
}

void ABlasterCharacter::PlayHitReactMontage()
{
	if (!Combat || !Combat->EquippedWeapon) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName;
		SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatorController, AActor* DamageCauser)
{
	if (bEliminated) return;

	float DamageToHealth = Damage;
	if (Shield > 0)
	{
		if (Shield >= Damage)
		{
			Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
			DamageToHealth = 0.f;
		}
		else
		{
			Shield = 0.f;
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth); // Health is replicated.

	UpdateHUDHealth();
	UpdateHUDShield();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if (BlasterGameMode)
		{
			BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(Controller);
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::UpdateHUDHealth()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>(Controller);
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>( Controller );
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDShield( Shield, MaxShield );
	}
}

void ABlasterCharacter::UpdateHUDAmmo()
{
	BlasterPlayerController = BlasterPlayerController ? BlasterPlayerController : Cast<ABlasterPlayerController>( Controller );
	if (BlasterPlayerController && Combat && Combat->EquippedWeapon)
	{
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
		BlasterPlayerController->SetHUDWeaponAmmo( Combat->EquippedWeapon->GetAmmo() );
	}
}

void ABlasterCharacter::PollInit()
{
	if (!BlasterPlayerState)
	{
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>(); // Pawn::GetPlayerState
		if (BlasterPlayerState)
		{
			// Add 0 score everry frame to existing score to update client's score faster.
			BlasterPlayerState->AddToScore(0.f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}

void ABlasterCharacter::Eliminated()
{
	/*
	* Some things we want to do only in the server.
	* This function is being called from GameMode class.
	*/
	DropOrDestroyWeapons();

	// Call multicast for logic in both server and clients.
	MulticastEliminated();
	
	GetWorldTimerManager().SetTimer(EliminatedTimer, this, &ABlasterCharacter::EliminatedTimerFinished, EliminationDelay);
}

void ABlasterCharacter::DropOrDestroyWeapons()
{
	if (Combat)
	{
		if (Combat->EquippedWeapon)
		{
			DropOrDestroyWeapon( Combat->EquippedWeapon );
		}

		if (Combat->SecondaryWeapon)
		{
			DropOrDestroyWeapon( Combat->SecondaryWeapon );
		}
	}
}

void ABlasterCharacter::DropOrDestroyWeapon( AWeapon* Weapon )
{
	if (!Weapon) return;
	
	if (Weapon->bDestroyWeapon)
	{
		Weapon->Destroy();
	}
	else
	{
		Weapon->Dropped();
	}
	
}

void ABlasterCharacter::MulticastEliminated_Implementation()
{
	if (BlasterPlayerController)
	{
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	bEliminated = true;
	PlayEliminatedMontage();

	// Start dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);

		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	// Disable character movement
	GetCharacterMovement()->DisableMovement();
	//GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn elimination bot 
	if (EliminationBotEffect)
	{
		FVector EliminationBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		EliminationBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), EliminationBotEffect, EliminationBotSpawnPoint, GetActorRotation());
	}
	if (EliminationBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(this, EliminationBotSound, GetActorLocation());
	}

	bool bHideSniperScope = IsLocallyControlled() && 
		Combat && 
		Combat->bAiming && 
		Combat->EquippedWeapon && 
		Combat->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
	if (bHideSniperScope)
	{
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::EliminatedTimerFinished()
{
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if (BlasterGameMode)
	{
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);

	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if (BlasterGameMode && World && !bEliminated && DefaultWeaponClass)
	{
		// We know we are in the server
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>( DefaultWeaponClass );
		if (Combat)
		{
			Combat->EquipWeapon( StartingWeapon );
			StartingWeapon->bDestroyWeapon = true;	// Only weapons that start with the player get destroyed. (Change this to a timer that destroys weapon after being dropped? )
		}
	}
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
	//if (!Combat) return nullptr;
	//return Combat->EquippedWeapon;
	return Combat ? Combat->EquippedWeapon : nullptr;
}

FVector ABlasterCharacter::GetHitTarget() const
{
	if (!Combat) return FVector();
	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
	if (!Combat) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* NewOverlappingWeapon)
{
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(false);
		}
	}

	OverlappingWeapon = NewOverlappingWeapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}
