// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NimbleTerminator/Interfaces/BulletHitInterface.h"
#include "Enemy.generated.h"

class UParticleSystem;
class USoundCue;

UCLASS()
class NIMBLETERMINATOR_API AEnemy : public ACharacter, public IBulletHitInterface
{
	GENERATED_BODY()

public:
	AEnemy();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BulletHit_Implementation(FHitResult HitResult) override;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USoundCue* ImpactSound;

	/** Stats */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Stats, meta = (AllowPrivateAccess = "true"))
	float Health { 100.f };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Stats, meta = (AllowPrivateAccess = "true"))
	float MaxHealth { 100.f };

public:

};
