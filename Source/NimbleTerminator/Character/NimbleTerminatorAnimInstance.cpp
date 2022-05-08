// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorAnimInstance.h"

#include "NimbleTerminatorCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UNimbleTerminatorAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (NimbleTerminatorCharacter == nullptr)
		NimbleTerminatorCharacter = Cast<ANimbleTerminatorCharacter>(TryGetPawnOwner());

	if (NimbleTerminatorCharacter == nullptr) return;

	// Get the lateral speed of the character from velocity
	FVector Velocity = NimbleTerminatorCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = NimbleTerminatorCharacter->GetCharacterMovement()->IsFalling();

	bIsAccelerating =
		NimbleTerminatorCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ?
			true :
			false;

	const FRotator AimRotation = NimbleTerminatorCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(NimbleTerminatorCharacter->GetVelocity());
	
	MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;

	if (NimbleTerminatorCharacter->GetVelocity().Size() > 0.f)
		LastMovementOffsetYaw = MovementOffsetYaw;

	bAiming = NimbleTerminatorCharacter->GetAiming();
}

void UNimbleTerminatorAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	NimbleTerminatorCharacter = Cast<ANimbleTerminatorCharacter>(TryGetPawnOwner());
}
