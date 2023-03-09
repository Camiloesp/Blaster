// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon/ProjectileWeapon.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Weapon/Projectile.h"

void AProjectileWeapon::Fire( const FVector& HitTarget )
{
	Super::Fire( HitTarget );

	APawn* InstigatorPawn = Cast<APawn>( GetOwner() );
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName( FName( "MuzzleFlash" ) );
	UWorld* World = GetWorld();
	if (MuzzleFlashSocket && World)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform( GetWeaponMesh() );
		// From muzzle flash socket to hit location from TraceUnderCrosshairs.
		FVector ToTarget = HitTarget - SocketTransform.GetLocation();
		FRotator TargetRotation = ToTarget.Rotation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		SpawnParams.Instigator = InstigatorPawn;

		AProjectile* SpawnedProjectile = nullptr;
		if (bUseServerSideRewind)
		{
			if (InstigatorPawn->HasAuthority()) // server
			{
				if (InstigatorPawn->IsLocallyControlled()) // server, host - Use replicated projectile
				{
					// Since we are server and host we dont need to use SSR even if bUseServerSideRewind is true.
					SpawnedProjectile = World->SpawnActor<AProjectile>( ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams );
					SpawnedProjectile->bUseServerSideRewind = false;
					SpawnedProjectile->Damage = Damage;
				}
				else // server, not locally controlled - Spawn non-replicated projectile, no SSR.
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>( ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams );
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
			else // cilent, using SSR
			{
				if (InstigatorPawn->IsLocallyControlled()) // Client, locally controlled - Spawn non-replicated projectile, use SSR.
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>( ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams );
					SpawnedProjectile->bUseServerSideRewind = true;
					SpawnedProjectile->TraceStart = SocketTransform.GetLocation();
					SpawnedProjectile->InitialVelocity = SpawnedProjectile->GetActorForwardVector() * SpawnedProjectile->InitialSpeed;
					SpawnedProjectile->Damage = Damage;
				}
				else // Client, not locally controlled - Spawn non-replicated projectile, no SSR
				{
					SpawnedProjectile = World->SpawnActor<AProjectile>( ServerSideRewindProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams );
					SpawnedProjectile->bUseServerSideRewind = false;
				}
			}
		}
		else // Weapon not using SSR
		{
			if (InstigatorPawn->HasAuthority())
			{
				SpawnedProjectile = World->SpawnActor<AProjectile>( ProjectileClass, SocketTransform.GetLocation(), TargetRotation, SpawnParams );
				SpawnedProjectile->bUseServerSideRewind = false;
				SpawnedProjectile->Damage = Damage;
			}
		}
	}
}
