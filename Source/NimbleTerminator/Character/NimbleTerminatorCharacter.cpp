// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ANimbleTerminatorCharacter::ANimbleTerminatorCharacter() :
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.f;
	// Rotate the arm based on the controller
	CameraBoom->bUsePawnControlRotation = true;

	// Create a Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// FollowCamera does not rotate relative to arm
	FollowCamera->bUsePawnControlRotation = false;
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
