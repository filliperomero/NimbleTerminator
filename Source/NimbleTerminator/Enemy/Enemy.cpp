// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"

#include "Blueprint/UserWidget.h"
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

	if (HitNumbers.Num() > 0)
		UpdateHitNumbers();
}

void AEnemy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &ThisClass::HideHealthBar, HealthBarDisplayTime);
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

	ShowHealthBar();
	PlayHitMontage(FName("HitReactFront"));
}

float AEnemy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (Health - DamageAmount <= 0.f)
	{
		Health = 0.f;
		Die();
	}
	else
	{
		Health -= DamageAmount;
	}

	return DamageAmount;
}

void AEnemy::Die()
{
	HideHealthBar();
}

void AEnemy::PlayHitMontage(FName Section, float PlayRate)
{
	if (HitMontage == nullptr || !bCanHitReact) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(HitMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, HitMontage);
	}

	bCanHitReact = false;
	GetWorldTimerManager().SetTimer(HitReactTimer, this, &ThisClass::ResetHitReactTimer, FMath::RandRange(HitReactTimeMin, HitReactTimeMax));
}

void AEnemy::ResetHitReactTimer()
{
	bCanHitReact = true;
}

void AEnemy::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
{
	HitNumbers.Add(HitNumber, Location);

	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelegate;
	
	HitNumberDelegate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	GetWorld()->GetTimerManager().SetTimer(HitNumberTimer, HitNumberDelegate, HitNumberDestroyTime, false);
}

void AEnemy::DestroyHitNumber(UUserWidget* HitNumber)
{
	HitNumbers.Remove(HitNumber);
	// This will remove from the viewport
	HitNumber->RemoveFromParent();
}

void AEnemy::UpdateHitNumbers()
{
	for (auto& HitPair : HitNumbers)
	{
		UUserWidget* HitNumber { HitPair.Key };
		const FVector Location { HitPair.Value };
		FVector2D ScreenPosition;

		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);

		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}


