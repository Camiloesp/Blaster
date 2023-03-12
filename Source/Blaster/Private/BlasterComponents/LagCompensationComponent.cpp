// Fill out your copyright notice in the Description page of Project Settings.


#include "BlasterComponents/LagCompensationComponent.h"
#include "Character/BlasterCharacter.h"
#include "Weapon/Weapon.h"
#include "Components/BoxComponent.h"
#include "Blaster/Blaster.h"
#include "Kismet/GameplayStatics.h"

// Sets default values for this component's properties
ULagCompensationComponent::ULagCompensationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void ULagCompensationComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void ULagCompensationComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	SaveFramePackage();
}

void ULagCompensationComponent::SaveFramePackage()
{
	if (!Character || !Character->HasAuthority()) return;
	if (FrameHistory.Num() <= 1)
	{
		FFramePackage ThisFrame;
		SaveFramePackage( ThisFrame );
		FrameHistory.AddHead( ThisFrame );
	}
	else
	{
		float HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		while (HistoryLength > MaxRecordTime)
		{
			FrameHistory.RemoveNode( FrameHistory.GetTail() );
			HistoryLength = FrameHistory.GetHead()->GetValue().Time - FrameHistory.GetTail()->GetValue().Time;
		}

		FFramePackage ThisFrame;
		SaveFramePackage( ThisFrame );
		FrameHistory.AddHead( ThisFrame );

		//ShowFramePackage( ThisFrame, FColor::Red );
	}
}

void ULagCompensationComponent::SaveFramePackage( FFramePackage& Package )
{
	Character = !Character ? Cast<ABlasterCharacter>( GetOwner() ) : Character;
	if (Character)
	{
		Package.Time = GetWorld()->GetTimeSeconds(); // This is the official server time since component is not replicated.
		Package.Character = Character;

		for (auto& BoxPair : Character->HitCollisionBoxes)
		{
			FBoxInformation BoxInformation;
			BoxInformation.Location = BoxPair.Value->GetComponentLocation();
			BoxInformation.Rotation = BoxPair.Value->GetComponentRotation();
			BoxInformation.BoxExtent = BoxPair.Value->GetScaledBoxExtent();

			Package.HitBoxInfo.Add( BoxPair.Key, BoxInformation );
		}
	}
}

FFramePackage ULagCompensationComponent::InterpBetweenFrames( const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime )
{
	const float Distance = YoungerFrame.Time - OlderFrame.Time;
	const float InterpFraction = FMath::Clamp( (HitTime - OlderFrame.Time) / Distance, 0.f, 1.f );

	FFramePackage InterpFramePackage;
	InterpFramePackage.Time = HitTime;

	for (auto& YoungerPair : YoungerFrame.HitBoxInfo)
	{
		// More efficient if not creating copies.
		const FName& BoxInfoName = YoungerPair.Key;

		const FBoxInformation& OlderBox = OlderFrame.HitBoxInfo[BoxInfoName];
		const FBoxInformation& YoungerBox = YoungerFrame.HitBoxInfo[BoxInfoName];

		FBoxInformation InterpBoxInfo;

		InterpBoxInfo.Location = FMath::VInterpTo( OlderBox.Location, YoungerBox.Location, 1.f, InterpFraction );
		InterpBoxInfo.Rotation = FMath::RInterpTo( OlderBox.Rotation, YoungerBox.Rotation, 1.f, InterpFraction );
		InterpBoxInfo.BoxExtent = YoungerBox.BoxExtent; // OlderBox.BoxExtent works too.

		InterpFramePackage.HitBoxInfo.Add( BoxInfoName, InterpBoxInfo );
	}

	return InterpFramePackage;
}

FServerSideRewindResult ULagCompensationComponent::ConfirmHit( const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation )
{
	if (!HitCharacter) return FServerSideRewindResult();

	FFramePackage CurrentFrame;
	CacheBoxPositions( HitCharacter, CurrentFrame ); // store box locations so we can move the back later.
	MoveBoxes( HitCharacter, Package );
	EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::NoCollision );

	// Enable collision for the head first.
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName( "head" )];
	HeadBox->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	HeadBox->SetCollisionResponseToChannel( ECC_HitBox, ECollisionResponse::ECR_Block );

	FHitResult ConfirmHitResult;
	const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f; // Extend hit location so its not on surface of collision
	UWorld* World = GetWorld();
	if (World)
	{
		World->LineTraceSingleByChannel(
			ConfirmHitResult,
			TraceStart,
			TraceEnd,
			ECC_HitBox
		);

		if (ConfirmHitResult.bBlockingHit) // we hit the head, return early.
		{
			if (ConfirmHitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>( ConfirmHitResult.Component );
				if (Box)
				{
					//DrawDebugBox( GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat(Box->GetComponentRotation()), FColor::Red, false, 8.f );
				}
			}
			ResetHitBoxes( HitCharacter, CurrentFrame );
			EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::QueryAndPhysics );
			return FServerSideRewindResult{ true, true };
		}
		else // Didn't hit the head, check the rest of the boxes
		{
			for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
			{
				if (HitBoxPair.Value)
				{
					HitBoxPair.Value->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
					HitBoxPair.Value->SetCollisionResponseToChannel( ECC_HitBox, ECollisionResponse::ECR_Block );
				}
			}

			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			if (ConfirmHitResult.bBlockingHit)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>( ConfirmHitResult.Component );
					if (Box)
					{
						//DrawDebugBox( GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat( Box->GetComponentRotation() ), FColor::Blue, false, 8.f );
					}
				}
				ResetHitBoxes( HitCharacter, CurrentFrame );
				EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::QueryAndPhysics );
				return FServerSideRewindResult{ true, false };
			}
		}
	}

	ResetHitBoxes( HitCharacter, CurrentFrame );
	EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::QueryAndPhysics );
	return FServerSideRewindResult{ false, false };
}

FServerSideRewindResult ULagCompensationComponent::ProjectileConfirmHit( const FFramePackage& Package, ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime )
{
	FFramePackage CurrentFrame;
	CacheBoxPositions( HitCharacter, CurrentFrame ); // store box locations so we can move the back later.
	MoveBoxes( HitCharacter, Package );
	EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::NoCollision );

	// Enable collision for the head first.
	UBoxComponent* HeadBox = HitCharacter->HitCollisionBoxes[FName( "head" )];
	HeadBox->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
	HeadBox->SetCollisionResponseToChannel( ECC_HitBox, ECollisionResponse::ECR_Block );

	FPredictProjectilePathParams PathParams;
	PathParams.bTraceWithCollision = true;
	PathParams.MaxSimTime = MaxRecordTime;
	PathParams.LaunchVelocity = InitialVelocity;
	PathParams.StartLocation = TraceStart;
	PathParams.SimFrequency = 15.f;
	PathParams.ProjectileRadius = 5.f;
	PathParams.TraceChannel = ECC_HitBox;
	PathParams.ActorsToIgnore.Add( GetOwner() );
	//PathParams.DrawDebugTime = 5.f;
	//PathParams.DrawDebugType = EDrawDebugTrace::ForDuration;

	FPredictProjectilePathResult PathResult;
	UGameplayStatics::PredictProjectilePath( this, PathParams, PathResult );

	if (PathResult.HitResult.bBlockingHit) // Hit the head, return early.
	{
		if (PathResult.HitResult.Component.IsValid())
		{
			UBoxComponent* Box = Cast<UBoxComponent>( PathResult.HitResult.Component );
			if (Box)
			{
				//DrawDebugBox( GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat( Box->GetComponentRotation() ), FColor::Red, false, 8.f );
			}
		}
		ResetHitBoxes( HitCharacter, CurrentFrame );
		EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::QueryAndPhysics );
		return FServerSideRewindResult{ true, true };
	}
	else // We didn't hit the head, check the rest of the boxes.
	{
		for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
		{
			if (HitBoxPair.Value)
			{
				HitBoxPair.Value->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
				HitBoxPair.Value->SetCollisionResponseToChannel( ECC_HitBox, ECollisionResponse::ECR_Block );
			}
		}

		UGameplayStatics::PredictProjectilePath( this, PathParams, PathResult );

		if (PathResult.HitResult.bBlockingHit)
		{
			if (PathResult.HitResult.Component.IsValid())
			{
				UBoxComponent* Box = Cast<UBoxComponent>( PathResult.HitResult.Component );
				if (Box)
				{
					//DrawDebugBox( GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat( Box->GetComponentRotation() ), FColor::Blue, false, 8.f );
				}
			}
			ResetHitBoxes( HitCharacter, CurrentFrame );
			EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::QueryAndPhysics );
			return FServerSideRewindResult{ true, false };
		}
	}

	ResetHitBoxes( HitCharacter, CurrentFrame );
	EnableCharacterMeshCollision( HitCharacter, ECollisionEnabled::QueryAndPhysics );
	return FServerSideRewindResult{ false, false };
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunConfirmHit( const TArray<FFramePackage>& FramePackages, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations )
{
	for (auto& Frame : FramePackages)
	{
		if (!Frame.Character) return FShotgunServerSideRewindResult();
	}

	FShotgunServerSideRewindResult ShotgunResult;
	TArray<FFramePackage> CurrentFrames;
	for (auto& Frame : FramePackages)
	{
		FFramePackage CurrentFrame;
		CurrentFrame.Character = Frame.Character;
		CacheBoxPositions( Frame.Character, CurrentFrame ); // store box locations so we can move the back later.
		MoveBoxes( Frame.Character, Frame );
		EnableCharacterMeshCollision( Frame.Character, ECollisionEnabled::NoCollision );

		CurrentFrames.Add( CurrentFrame );
	}
	// Looping through the same FramePackages as above but same amount of actions.
	for (auto& Frame : FramePackages)
	{
		// Enable collision for the head first.
		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName( "head" )];
		HeadBox->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
		HeadBox->SetCollisionResponseToChannel( ECC_HitBox, ECollisionResponse::ECR_Block );
	}

	// Check for headshots
	UWorld* World = GetWorld();
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f; // Extend hit location so its not on surface of collision
		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>( ConfirmHitResult.GetActor() );
			if (BlasterCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>( ConfirmHitResult.Component );
					if (Box)
					{
						//DrawDebugBox( GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat( Box->GetComponentRotation() ), FColor::Red, false, 8.f );
					}
				}

				if (ShotgunResult.HeadShots.Contains( BlasterCharacter ))
				{
					ShotgunResult.HeadShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.HeadShots.Emplace( BlasterCharacter, 1 );
				}
			}
		}
	}

	// Enable Collision for all boxes, then disable for headbox
	for (auto& Frame : FramePackages)
	{
		for (auto& HitBoxPair : Frame.Character->HitCollisionBoxes)
		{
			if (HitBoxPair.Value)
			{
				HitBoxPair.Value->SetCollisionEnabled( ECollisionEnabled::QueryAndPhysics );
				HitBoxPair.Value->SetCollisionResponseToChannel( ECC_HitBox, ECollisionResponse::ECR_Block );
			}
		}

		UBoxComponent* HeadBox = Frame.Character->HitCollisionBoxes[FName( "head" )];
		HeadBox->SetCollisionEnabled( ECollisionEnabled::NoCollision );
	}

	// Check for body shots
	for (auto& HitLocation : HitLocations)
	{
		FHitResult ConfirmHitResult;
		const FVector TraceEnd = TraceStart + (HitLocation - TraceStart) * 1.25f; // Extend hit location so its not on surface of collision
		if (World)
		{
			World->LineTraceSingleByChannel(
				ConfirmHitResult,
				TraceStart,
				TraceEnd,
				ECC_HitBox
			);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>( ConfirmHitResult.GetActor() );
			if (BlasterCharacter)
			{
				if (ConfirmHitResult.Component.IsValid())
				{
					UBoxComponent* Box = Cast<UBoxComponent>( ConfirmHitResult.Component );
					if (Box)
					{
						//DrawDebugBox( GetWorld(), Box->GetComponentLocation(), Box->GetScaledBoxExtent(), FQuat( Box->GetComponentRotation() ), FColor::Blue, false, 8.f );
					}
				}

				if (ShotgunResult.BodyShots.Contains( BlasterCharacter ))
				{
					ShotgunResult.BodyShots[BlasterCharacter]++;
				}
				else
				{
					ShotgunResult.BodyShots.Emplace( BlasterCharacter, 1 );
				}
			}
		}
	}

	for (auto& Frame : CurrentFrames)
	{
		ResetHitBoxes( Frame.Character, Frame );
		EnableCharacterMeshCollision( Frame.Character, ECollisionEnabled::QueryAndPhysics );
	}

	return ShotgunResult;
}

void ULagCompensationComponent::CacheBoxPositions( ABlasterCharacter* HitCharacter, FFramePackage& OutFramePackage )
{
	if (!HitCharacter) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value)
		{
			FBoxInformation BoxInfo;
			BoxInfo.Location = HitBoxPair.Value->GetComponentLocation();
			BoxInfo.Rotation = HitBoxPair.Value->GetComponentRotation();
			BoxInfo.BoxExtent = HitBoxPair.Value->GetScaledBoxExtent();

			OutFramePackage.HitBoxInfo.Add( HitBoxPair.Key, BoxInfo );
		}
	}
}

void ULagCompensationComponent::MoveBoxes( ABlasterCharacter* HitCharacter, const FFramePackage& Package )
{
	if (!HitCharacter) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value)
		{
			HitBoxPair.Value->SetWorldLocation( Package.HitBoxInfo[HitBoxPair.Key].Location );
			HitBoxPair.Value->SetWorldRotation( Package.HitBoxInfo[HitBoxPair.Key].Rotation );
			HitBoxPair.Value->SetBoxExtent( Package.HitBoxInfo[HitBoxPair.Key].BoxExtent );
		}
	}
}

void ULagCompensationComponent::ResetHitBoxes( ABlasterCharacter* HitCharacter, const FFramePackage& Package )
{
	if (!HitCharacter) return;

	for (auto& HitBoxPair : HitCharacter->HitCollisionBoxes)
	{
		if (HitBoxPair.Value)
		{
			HitBoxPair.Value->SetWorldLocation( Package.HitBoxInfo[HitBoxPair.Key].Location );
			HitBoxPair.Value->SetWorldRotation( Package.HitBoxInfo[HitBoxPair.Key].Rotation );
			HitBoxPair.Value->SetBoxExtent( Package.HitBoxInfo[HitBoxPair.Key].BoxExtent );

			HitBoxPair.Value->SetCollisionEnabled( ECollisionEnabled::NoCollision );
		}
	}
}

void ULagCompensationComponent::EnableCharacterMeshCollision( ABlasterCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled )
{
	if (HitCharacter && HitCharacter->GetMesh())
	{
		HitCharacter->GetMesh()->SetCollisionEnabled( CollisionEnabled );
	}
}

void ULagCompensationComponent::ShowFramePackage( const FFramePackage& Package, const FColor Color )
{
	for (auto& BoxInfo : Package.HitBoxInfo)
	{
		DrawDebugBox( GetWorld(), BoxInfo.Value.Location, BoxInfo.Value.BoxExtent, FQuat( BoxInfo.Value.Rotation ), Color, false, 4.f ); // if called on tick set persistent lines to false. and add a lifetime
	}
}

FServerSideRewindResult ULagCompensationComponent::ServerSideRewind( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime )
{
	FFramePackage FrameToCheck = GetFrameToCheck( HitCharacter, HitTime );
	return ConfirmHit( FrameToCheck, HitCharacter, TraceStart, HitLocation );
}

FServerSideRewindResult ULagCompensationComponent::ProjectileServerSideRewind( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime )
{
	FFramePackage FrameToCheck = GetFrameToCheck( HitCharacter, HitTime );
	return ProjectileConfirmHit( FrameToCheck, HitCharacter, TraceStart, InitialVelocity, HitTime );
}

FShotgunServerSideRewindResult ULagCompensationComponent::ShotgunServerSideRewind( const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime )
{
	TArray<FFramePackage> FramesToCheck;
	for (ABlasterCharacter* HitCharacter : HitCharacters)
	{
		FramesToCheck.Add( GetFrameToCheck( HitCharacter, HitTime ) );
	}

	return ShotgunConfirmHit( FramesToCheck, TraceStart, HitLocations );
}

FFramePackage ULagCompensationComponent::GetFrameToCheck( ABlasterCharacter* HitCharacter, float HitTime )
{
	bool bReturn =
		!HitCharacter ||
		!HitCharacter->GetLagCompensation() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetHead() ||
		!HitCharacter->GetLagCompensation()->FrameHistory.GetTail();

	if (bReturn) return FFramePackage();

	// Frame package that we check to verify a hit.
	FFramePackage FrameToCheck;
	bool bShouldInterpolate = true;
	// This is the HitCharacter frame history, not the history from this component.
	const TDoubleLinkedList<FFramePackage>& History = HitCharacter->GetLagCompensation()->FrameHistory;
	const float OldestHistoryTime = History.GetTail()->GetValue().Time;
	const float NewestHistoryTime = History.GetHead()->GetValue().Time;
	if (OldestHistoryTime > HitTime)
	{
		// Too far back - Too laggy to do SSR (ServerSideRewind)
		return FFramePackage();
	}
	if (OldestHistoryTime == HitTime)
	{
		FrameToCheck = History.GetTail()->GetValue();
		bShouldInterpolate = false;
	}
	if (NewestHistoryTime <= HitTime)
	{
		FrameToCheck = History.GetHead()->GetValue();
		bShouldInterpolate = false;
	}

	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Younger = History.GetHead(); // YoungerNode
	TDoubleLinkedList<FFramePackage>::TDoubleLinkedListNode* Older = Younger; // OlderNode
	while (Older->GetValue().Time > HitTime) // Is older still younger than HitTime
	{
		// March back until: OlderTime < HitTime < YoungerTime
		if (!Older->GetNextNode()) break;
		Older = Older->GetNextNode();
		if (Older->GetValue().Time > HitTime)
		{
			Younger = Older;
		}
	}
	if (Older->GetValue().Time == HitTime) // Highly unlikely(because floating points are rarely equal), but we found our frame to check
	{
		FrameToCheck = Older->GetValue();
		bShouldInterpolate = false;
	}
	if (bShouldInterpolate)
	{
		// Interpolate between younger and older.
		FrameToCheck = InterpBetweenFrames( Older->GetValue(), Younger->GetValue(), HitTime );
	}

	FrameToCheck.Character = HitCharacter;
	return FrameToCheck;
}

void ULagCompensationComponent::ServerScoreRequest_Implementation( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize& HitLocation, float HitTime )
{
	FServerSideRewindResult Confirm = ServerSideRewind( HitCharacter, TraceStart, HitLocation, HitTime );

	if (Character && HitCharacter && Character->GetEquippedWeapon() && Confirm.bHitConfirmed)
	{
		const float Damage = Confirm.bHeadshot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		UGameplayStatics::ApplyDamage( HitCharacter, Damage, Character->Controller, Character->GetEquippedWeapon(), UDamageType::StaticClass() );
	}
}

void ULagCompensationComponent::ProjectileServerScoreRequest_Implementation( ABlasterCharacter* HitCharacter, const FVector_NetQuantize& TraceStart, const FVector_NetQuantize100& InitialVelocity, float HitTime )
{
	FServerSideRewindResult Confirm = ProjectileServerSideRewind( HitCharacter, TraceStart, InitialVelocity, HitTime );

	if (Character && HitCharacter && Confirm.bHitConfirmed && Character->GetEquippedWeapon())
	{
		const float Damage = Confirm.bHeadshot ? Character->GetEquippedWeapon()->GetHeadShotDamage() : Character->GetEquippedWeapon()->GetDamage();

		UGameplayStatics::ApplyDamage( HitCharacter, Damage, Character->Controller, HitCharacter->GetEquippedWeapon(), UDamageType::StaticClass());
	}
}

void ULagCompensationComponent::ShotgunServerScoreRequest_Implementation( const TArray<ABlasterCharacter*>& HitCharacters, const FVector_NetQuantize& TraceStart, const TArray<FVector_NetQuantize>& HitLocations, float HitTime )
{
	FShotgunServerSideRewindResult Confirm = ShotgunServerSideRewind( HitCharacters, TraceStart, HitLocations, HitTime );

	for (auto& HitCharacter : HitCharacters)
	{
		if (!HitCharacter || !HitCharacter->GetEquippedWeapon() || !Character) continue;

		float TotalDamage = 0.f;

		if (Confirm.HeadShots.Contains( HitCharacter ))
		{
			float HeadShotDamage = Confirm.HeadShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetHeadShotDamage();
			TotalDamage += HeadShotDamage;
		}
		if (Confirm.BodyShots.Contains( HitCharacter ))
		{
			float BodyShotDamage = Confirm.BodyShots[HitCharacter] * HitCharacter->GetEquippedWeapon()->GetDamage();
			TotalDamage += BodyShotDamage;
		}

		UGameplayStatics::ApplyDamage( HitCharacter, TotalDamage, Character->Controller, HitCharacter->GetEquippedWeapon(), UDamageType::StaticClass() );
	}
}
