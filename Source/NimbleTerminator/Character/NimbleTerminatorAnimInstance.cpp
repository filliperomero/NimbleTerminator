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

	bReloading = NimbleTerminatorCharacter->GetCombatState() == ECombatState::ECS_Reloading;

	TurnInPlace();
}

void UNimbleTerminatorAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	NimbleTerminatorCharacter = Cast<ANimbleTerminatorCharacter>(TryGetPawnOwner());
}

void UNimbleTerminatorAnimInstance::TurnInPlace()
{
	if (NimbleTerminatorCharacter == nullptr) return;

	Pitch = NimbleTerminatorCharacter->GetBaseAimRotation().Pitch;

	if (Speed > 0.f)
	{
		RootYawOffset = 0.f;
		CharacterYaw = NimbleTerminatorCharacter->GetActorRotation().Yaw;
		CharacterYawLastFrame = CharacterYaw;
		RotationCurve = 0.f;
		RotationCurveLastFrame = 0.f;

		return;
	}

	CharacterYawLastFrame = CharacterYaw;
	CharacterYaw = NimbleTerminatorCharacter->GetActorRotation().Yaw;
	// Difference between CharacterYaw and CharacterYawLastFrame
	const float YawDelta = CharacterYaw - CharacterYawLastFrame;

	// Root Yaw offset, updated and clamped to [-180, 180]
	RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - YawDelta);

	// 1.0 if turning, 0.0 if not (Curve created in the turning animations)
	const float Turning = GetCurveValue(TEXT("Turning"));
	if (Turning > 0.f)
	{
		RotationCurveLastFrame = RotationCurve;
		RotationCurve = GetCurveValue(TEXT("Rotation"));

		const float DeltaRotation = RotationCurve - RotationCurveLastFrame;

		// RootYawOffset > 0 -> Turning Left, RootYawOffset < 0 -> Turning Right
		RootYawOffset > 0.f ? RootYawOffset -= DeltaRotation : RootYawOffset += DeltaRotation;

		const float ABSRootYawOffset = FMath::Abs(RootYawOffset);
		if (ABSRootYawOffset > 90.f)
		{
			// Compensate the value by subtracting or adding the excess
			const float YawExcess = ABSRootYawOffset - 90.f;
			RootYawOffset > 0.f ? RootYawOffset -= YawExcess : RootYawOffset += YawExcess;
		}
	}
}