// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
}

void AEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::BulletHit_Implementation(FHitResult HitResult)
{
	if (ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());

	if (ImpactParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ImpactParticles,
			HitResult.Location,
			FRotator(0.f),
			true
			);
	}
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;		
	}
	else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}

