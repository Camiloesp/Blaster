// Fill out your copyright notice in the Description page of Project Settings.


#include "CaptureTheFlag/FlagZone.h"
#include "Components/SphereComponent.h"
#include "Weapon/Flag.h"
#include "GameModes/CaptureTheFlagGameMode.h"

// Sets default values
AFlagZone::AFlagZone()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	ZoneSphere = CreateDefaultSubobject<USphereComponent>( TEXT("ZoneSphere"));
	SetRootComponent( ZoneSphere );
}

// Called when the game starts or when spawned
void AFlagZone::BeginPlay()
{
	Super::BeginPlay();

	ZoneSphere->OnComponentBeginOverlap.AddDynamic( this, &AFlagZone::OnSphereOverlap );
}

// Called every frame
void AFlagZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AFlagZone::OnSphereOverlap( UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult )
{
	AFlag* OverlappingFlag = Cast<AFlag>(OtherActor);
	if (OverlappingFlag && OverlappingFlag->GetTeam() != Team)
	{
		ACaptureTheFlagGameMode* GameMode = GetWorld()->GetAuthGameMode<ACaptureTheFlagGameMode>();
		if (GameMode) // Server
		{
			GameMode->FlagCaptured( OverlappingFlag, this );	
		}
		OverlappingFlag->ResetFlag();
	}
}

