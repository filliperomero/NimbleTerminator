// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy.h"

#include "EnemyController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "NimbleTerminator/Character/NimbleTerminatorCharacter.h"
#include "Sound/SoundCue.h"

AEnemy::AEnemy()
{
	PrimaryActorTick.bCanEverTick = true;

	AggroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AggroSphere"));
	AggroSphere->SetupAttachment(GetRootComponent());
	AggroSphere->SetSphereRadius(450.f);
	
	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatRangeSphere"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());
	CombatRangeSphere->SetSphereRadius(180.f);

	// Construct left/right weapon collision boxes
	LeftWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Left Weapon Box"));
	LeftWeaponCollision->SetupAttachment(GetMesh(), FName("LeftWeaponBone"));

	RightWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Weapon Box"));
	RightWeaponCollision->SetupAttachment(GetMesh(), FName("RightWeaponBone"));
}

void AEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (AggroSphere)
		AggroSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::AggroSphereOverlap);

	if (CombatRangeSphere)
	{
		CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::CombatRangeOverlap);
		CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::CombatRangeEndOverlap);
	}

	if (LeftWeaponCollision)
	{
		LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnLeftWeaponOverlap);
		// Set collision presets for weapon boxes
		LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		LeftWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		LeftWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		LeftWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}

	if (RightWeaponCollision)
	{
		RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnRightWeaponOverlap);
		// Set collision presets for weapon boxes
		RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RightWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
		RightWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		RightWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	}

	if (GetMesh())
	{
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	}
	
	if (GetCapsuleComponent())
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	EnemyController = Cast<AEnemyController>(GetController());
	
	// Local space to World space
	const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
	const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);

	if (EnemyController && EnemyController->GetBlackboard())
	{
		EnemyController->GetBlackboard()->SetValueAsVector(TEXT("PatrolPoint"), WorldPatrolPoint);
		EnemyController->GetBlackboard()->SetValueAsVector(TEXT("PatrolPoint2"), WorldPatrolPoint2);
		EnemyController->RunBehaviorTree(BehaviorTree);
	}
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
	const float Stunned = FMath::FRandRange(0.f, 1.f);
	if (Stunned <= StunChance)
	{
		PlayHitMontage(FName("HitReactFront"));
		SetStunned(true);
	}
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

void AEnemy::AggroSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr) return;
	
	auto Character = Cast<ANimbleTerminatorCharacter>(OtherActor);
	if (Character && EnemyController && EnemyController->GetBlackboard())
	{
		EnemyController->GetBlackboard()->SetValueAsObject(TEXT("Target"), Character);
	}
}

void AEnemy::CombatRangeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == nullptr) return;

	auto Character = Cast<ANimbleTerminatorCharacter>(OtherActor);

	if (Character)
	{
		bInAttackRange = true;
		
		if (EnemyController && EnemyController->GetBlackboard())
			EnemyController->GetBlackboard()->SetValueAsBool(TEXT("InAttackRange"), true);
	}
}

void AEnemy::CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor == nullptr) return;

	auto Character = Cast<ANimbleTerminatorCharacter>(OtherActor);

	if (Character)
	{
		bInAttackRange = false;
		
		if (EnemyController && EnemyController->GetBlackboard())
			EnemyController->GetBlackboard()->SetValueAsBool(TEXT("InAttackRange"), false);
	}
}

void AEnemy::SetStunned(bool Stunned)
{
	bStunned = Stunned;
	if (EnemyController && EnemyController->GetBlackboard())
		EnemyController->GetBlackboard()->SetValueAsBool(TEXT("Stunned"), Stunned);
}

void AEnemy::PlayAttackMontage(FName Section, float PlayRate)
{
	if (AttackMontage == nullptr) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(AttackMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, AttackMontage);
	}
}

FName AEnemy::GetAttackSectionName()
{
	FName SectionName;
	const int32 Section = FMath::RandRange(1, 4);
	
	switch (Section)
	{
	case 1:
		SectionName = AttackLFast;
		break;
	case 2:
		SectionName = AttackRFast;
		break;
	case 3:
		SectionName = AttackL;
		break;
	case 4:
		SectionName = AttackR;
		break;
	default:
		SectionName = AttackLFast;
	}

	return SectionName;
}

void AEnemy::DoDamage(ANimbleTerminatorCharacter* Victim)
{
	if (Victim == nullptr) return;
	
	UGameplayStatics::ApplyDamage(Victim, BaseDamage, EnemyController, this, UDamageType::StaticClass());

	if (Victim->GetMeleeImpactSound())
		UGameplayStatics::PlaySoundAtLocation(this, Victim->GetMeleeImpactSound(), GetActorLocation());
}

void AEnemy::SpawnBlood(ANimbleTerminatorCharacter* Victim, FName SocketName)
{
	const USkeletalMeshSocket* TipSocket { GetMesh()->GetSocketByName(SocketName) };
	if (TipSocket)
	{
		const FTransform SocketTransform = TipSocket->GetSocketTransform(GetMesh());
		if (Victim->GetBloodParticles())
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Victim->GetBloodParticles(), SocketTransform);
	}
}

void AEnemy::OnLeftWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Character = Cast<ANimbleTerminatorCharacter>(OtherActor);
	if (Character)
	{
		DoDamage(Character);
		SpawnBlood(Character, LeftWeaponSocket);
	}
}

void AEnemy::OnRightWeaponOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	auto Character = Cast<ANimbleTerminatorCharacter>(OtherActor);
	if (Character)
	{
		DoDamage(Character);
		SpawnBlood(Character, RightWeaponSocket);
	}
}

void AEnemy::ActivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEnemy::ActivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void AEnemy::DeactivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


