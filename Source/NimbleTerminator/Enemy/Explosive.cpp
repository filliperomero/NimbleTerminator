// Fill out your copyright notice in the Description page of Project Settings.


#include "Explosive.h"

#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AExplosive::AExplosive()
{
	PrimaryActorTick.bCanEverTick = true;

	ExplosiveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExplosiveMesh"));
	SetRootComponent(ExplosiveMesh);

	ExplosionRadiusSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ExplosionRadiusSphere"));
	ExplosionRadiusSphere->SetupAttachment(GetRootComponent());
	ExplosionRadiusSphere->SetSphereRadius(250.f);
}

void AExplosive::BeginPlay()
{
	Super::BeginPlay();
}

void AExplosive::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AExplosive::BulletHit_Implementation(FHitResult HitResult, AActor* Shooter, AController* ShooterController)
{
	if (ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());

	if (ExplodeParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ExplodeParticles,
			HitResult.Location,
			FRotator(0.f),
			true
			);
	}

	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

	// TODO: check to see if we have applyDamage in Radius
	for (auto Actor : OverlappingActors)
	{
		UGameplayStatics::ApplyDamage(Actor, BaseDamage, ShooterController, Shooter, UDamageType::StaticClass());
	}
	
	Destroy();
}
