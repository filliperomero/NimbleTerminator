// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorCharacter.h"

#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

ANimbleTerminatorCharacter::ANimbleTerminatorCharacter() :
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 50.f, 50.f);

	// Create a Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // FollowCamera does not rotate relative to arm

	// Don't rotate when the controller rotates. Let the controller only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;
}

void ANimbleTerminatorCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ANimbleTerminatorCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANimbleTerminatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &ThisClass::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ThisClass::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &ThisClass::FireWeapon);
}

void ANimbleTerminatorCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || Value == 0.f) return;
	
	const FRotator Rotation{ Controller->GetControlRotation() };
	const FRotator YawRotation{ 0.f, Rotation.Yaw, 0.f };
	const FVector Direction{ FRotationMatrix{ YawRotation }.GetUnitAxis(EAxis::X) };
	
	AddMovementInput(Direction, Value);
}

void ANimbleTerminatorCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || Value == 0.f) return;

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));

	AddMovementInput(Direction, Value);
}

void ANimbleTerminatorCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANimbleTerminatorCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ANimbleTerminatorCharacter::FireWeapon()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}

	const USkeletalMeshSocket* BarrelSocket = GetMesh()->GetSocketByName(FName("BarrelSocket"));

	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(GetMesh());

		UWorld* World = GetWorld();
		if (World)
		{
			if (MuzzleFlash)
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
			}

			FHitResult FireHit;
			const FVector Start{ SocketTransform.GetLocation() };
			const FQuat Rotation{ SocketTransform.GetRotation() };
			const FVector RotationAxis{ Rotation.GetAxisX() };
			const FVector End{ Start + RotationAxis * TRACE_LENGTH };

			FVector BeamEndPoint{ End };
			
			World->LineTraceSingleByChannel(FireHit, Start, End, ECollisionChannel::ECC_Visibility);

			if (FireHit.bBlockingHit)
			{
				DrawDebugLine(World, Start, End, FColor::Red, false, 5.f);
				DrawDebugPoint(World, FireHit.Location, 5.f, FColor::Red, false, 5.f);

				BeamEndPoint = FireHit.ImpactPoint;

				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(World, ImpactParticles, FireHit.ImpactPoint);
				}
			}
			else
			{
				FireHit.ImpactPoint = BeamEndPoint;
			}

			if (BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(World, BeamParticles, SocketTransform);

				if (Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
				}
			}
		}
	}

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}
