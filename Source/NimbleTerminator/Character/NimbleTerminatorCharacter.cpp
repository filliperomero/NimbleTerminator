// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

ANimbleTerminatorCharacter::ANimbleTerminatorCharacter()
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
}

