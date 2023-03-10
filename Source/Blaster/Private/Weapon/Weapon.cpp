// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Character/BlasterCharacter.h"
#include "PlayerController/BlasterPlayerController.h"
#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/Casing.h"
#include "Engine/SkeletalMeshSocket.h"
#include "BlasterComponents/CombatComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AWeapon::AWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EnableCustomDepth(true);
	WeaponMesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_BLUE);
	WeaponMesh->MarkRenderStateDirty(); // <-- Refresh. each time we change the CustomDepthStencilValue ^, we want to make sure that that change results in updating this post process highlight material for the higlight on the weapon.

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(RootComponent);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AWeapon, WeaponState);
	DOREPLIFETIME_CONDITION( AWeapon, bUseServerSideRewind, COND_OwnerOnly ); // This will only replicate to the owner of the weapon to see if the player's ping is too high to use SSR
}

// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();
	
	// Set AreaSphere collision in server side only. Never mind....
	AreaSphere->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	AreaSphere->SetCollisionResponseToChannel( ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap );
	AreaSphere->OnComponentBeginOverlap.AddDynamic( this, &AWeapon::OnSphereOverlap );
	AreaSphere->OnComponentEndOverlap.AddDynamic( this, &AWeapon::OnSphereEndOverlap );
	
	if (PickupWidget) PickupWidget->SetVisibility(false);
}

// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::SetWeaponState(EWeaponState NewWeaponState)
{
	// Changing WeaponState will call OnRep_WeaponState autmatically to change it on clients.
	WeaponState = NewWeaponState;
	OnWeaponStateSet();
}

void AWeapon::OnWeaponStateSet()
{
	switch (WeaponState)
	{
		case EWeaponState::EWS_Equipped:
			OnEquipped();
			break;
		case EWeaponState::EWS_EquippedSecondary:
			OnEquippedSecondary();
			break;
		case EWeaponState::EWS_Dropped:
			OnDropped();
			break;
	}
}

void AWeapon::OnRep_WeaponState()
{
	OnWeaponStateSet();
}

void AWeapon::OnEquipped()
{
	ShowPickupWidget( false );
	AreaSphere->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	WeaponMesh->SetSimulatePhysics( false );
	WeaponMesh->SetEnableGravity( false );
	WeaponMesh->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
		WeaponMesh->SetEnableGravity( true );
		WeaponMesh->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
	}
	EnableCustomDepth( false );

	BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>( GetOwner() ) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && bUseServerSideRewind)
	{
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>( BlasterOwnerCharacter->Controller ) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority() && !BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.AddDynamic( this, &AWeapon::OnPingTooHigh );
		}
	}
}

void AWeapon::OnDropped()
{
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled( ECollisionEnabled::QueryOnly );
	}
	WeaponMesh->SetSimulatePhysics( true );
	WeaponMesh->SetEnableGravity( true );
	WeaponMesh->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	WeaponMesh->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Block );
	WeaponMesh->SetCollisionResponseToChannel( ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore );
	WeaponMesh->SetCollisionResponseToChannel( ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore );

	WeaponMesh->SetCustomDepthStencilValue( CUSTOM_DEPTH_BLUE );
	WeaponMesh->MarkRenderStateDirty();
	EnableCustomDepth( true );

	BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>( GetOwner() ) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter) // bUseServerSideRewind <- removed
	{
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>( BlasterOwnerCharacter->Controller ) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.RemoveDynamic( this, &AWeapon::OnPingTooHigh );
		}
	}
}

void AWeapon::OnEquippedSecondary()
{
	ShowPickupWidget( false );
	AreaSphere->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	WeaponMesh->SetSimulatePhysics( false );
	WeaponMesh->SetEnableGravity( false );
	WeaponMesh->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	if (WeaponType == EWeaponType::EWT_SubmachineGun)
	{
		WeaponMesh->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
		WeaponMesh->SetEnableGravity( true );
		WeaponMesh->SetCollisionResponseToAllChannels( ECollisionResponse::ECR_Ignore );
	}
	//EnableCustomDepth(true);
	if (WeaponMesh)
	{
		WeaponMesh->SetCustomDepthStencilValue( CUSTOM_DEPTH_TAN );
		WeaponMesh->MarkRenderStateDirty();
	}

	BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>( GetOwner() ) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter) //bUseServerSideRewind
	{
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>( BlasterOwnerCharacter->Controller ) : BlasterOwnerController;
		if (BlasterOwnerController && HasAuthority() && BlasterOwnerController->HighPingDelegate.IsBound())
		{
			BlasterOwnerController->HighPingDelegate.RemoveDynamic( this, &AWeapon::OnPingTooHigh );
		}
	}
}

void AWeapon::SetHUDAmmo()
{
	BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter)
	{
		BlasterOwnerController = !BlasterOwnerController ? Cast<ABlasterPlayerController>(BlasterOwnerCharacter->Controller) : BlasterOwnerController;
		if (BlasterOwnerController)
		{
			BlasterOwnerController->SetHUDWeaponAmmo(Ammo);
		}
	}
}

void AWeapon::EnableCustomDepth(bool bEnable)
{
	if (WeaponMesh)
	{
		WeaponMesh->SetRenderCustomDepth(bEnable);
	}
}

void AWeapon::SpendRound()
{
	Ammo = FMath::Clamp(Ammo-1, 0, MagCapacity);
	SetHUDAmmo();
	if (HasAuthority())
	{
		ClientUpdateAmmo( Ammo );
	}
	else
	{
		++Sequence;
	}
}

void AWeapon::ClientUpdateAmmo_Implementation( int32 ServerAmmo )
{
	if (HasAuthority()) return;
	// Make correction if we spend more round before server round trip
	// Server reconciliation
	Ammo = ServerAmmo;
	--Sequence;
	Ammo -= Sequence;
	SetHUDAmmo();
}

void AWeapon::AddAmmo( int32 AmmoToAdd )
{
	Ammo = FMath::Clamp( Ammo + AmmoToAdd, 0, MagCapacity );
	SetHUDAmmo();
	ClientAddAmmo(AmmoToAdd);
}

void AWeapon::ClientAddAmmo_Implementation( int32 AmmoToAdd )
{
	if (HasAuthority()) return;
	Ammo = FMath::Clamp( Ammo + AmmoToAdd, 0, MagCapacity );
	BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>(GetOwner()) : BlasterOwnerCharacter;
	if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetCombat() && IsFull())
	{
		BlasterOwnerCharacter->GetCombat()->JumpToShotgunEnd();
	}
	SetHUDAmmo();
}

void AWeapon::OnRep_Owner()
{
	Super::OnRep_Owner();
	if (Owner == nullptr)
	{
		BlasterOwnerCharacter = nullptr;
		BlasterOwnerController = nullptr;
	}
	else
	{
		BlasterOwnerCharacter = !BlasterOwnerCharacter ? Cast<ABlasterCharacter>( Owner ) : BlasterOwnerCharacter;
		if (BlasterOwnerCharacter && BlasterOwnerCharacter->GetEquippedWeapon() && BlasterOwnerCharacter->GetEquippedWeapon() == this)
		{
			SetHUDAmmo();
		}
	}
}

void AWeapon::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		if (WeaponType == EWeaponType::EFT_Flag && BlasterCharacter->GetTeam() == Team) return;
		if (BlasterCharacter->IsHoldingTheFlag()) return; // OPTIONAL
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		if (WeaponType == EWeaponType::EFT_Flag && BlasterCharacter->GetTeam() == Team) return;
		if (BlasterCharacter->IsHoldingTheFlag()) return; // OPTIONAL
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnPingTooHigh( bool bPingTooHigh )
{
	bUseServerSideRewind = !bPingTooHigh;
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget) PickupWidget->SetVisibility(bShowWidget);
}

void AWeapon::Fire(const FVector& HitTarget)
{
	if (FireAnimation)
	{
		WeaponMesh->PlayAnimation(FireAnimation, false);
	}

	if (CasingClass)
	{
		const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
		if (AmmoEjectSocket)
		{
			FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
			
			UWorld* World = GetWorld();
			if (World)
			{
				World->SpawnActor<ACasing>(CasingClass, SocketTransform.GetLocation(), SocketTransform.GetRotation().Rotator());
			}
		}
	}
	SpendRound();
}

void AWeapon::Dropped()
{
	SetWeaponState(EWeaponState::EWS_Dropped);
	FDetachmentTransformRules DetachRules(EDetachmentRule::KeepWorld, true);
	WeaponMesh->DetachFromComponent(DetachRules);
	SetOwner(nullptr);
	BlasterOwnerCharacter = nullptr;
	BlasterOwnerController = nullptr;
}

bool AWeapon::IsEmpty()
{
	return Ammo <= 0;
}

bool AWeapon::IsFull()
{
	return Ammo == MagCapacity;
}

FVector AWeapon::TraceEndWithScatter( const FVector& HitTarget )
{
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName( "MuzzleFlash" );
	if (!MuzzleFlashSocket) return FVector();

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform( GetWeaponMesh() );
	const FVector TraceStart = SocketTransform.GetLocation();

	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange( 0.f, SphereRadius );
	const FVector EndLoc = SphereCenter + RandVec;
	const FVector ToEndLoc = EndLoc - TraceStart;

	/*DrawDebugSphere( GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true );
	DrawDebugSphere( GetWorld(), EndLoc, 4.f, 12, FColor::Orange, true );
	DrawDebugLine(
		GetWorld(),
		TraceStart,
		FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
		FColor::Cyan,
		true
	);*/

	return FVector( TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size() );
}
