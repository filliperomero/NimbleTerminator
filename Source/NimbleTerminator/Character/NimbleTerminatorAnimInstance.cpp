// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorAnimInstance.h"

#include "NimbleTerminatorCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "NimbleTerminator/Weapon/Weapon.h"

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
	bEquipping = NimbleTerminatorCharacter->GetCombatState() == ECombatState::ECS_Equipping;
	bCrouching = NimbleTerminatorCharacter->IsCrouching();
	bShouldUseFABRIK = NimbleTerminatorCharacter->GetCombatState() == ECombatState::ECS_Unoccupied ||
		NimbleTerminatorCharacter->GetCombatState() == ECombatState::ECS_FireTimerInProgress;

	if (NimbleTerminatorCharacter->GetEquippedWeapon())
		EquippedWeaponType = NimbleTerminatorCharacter->GetEquippedWeapon()->GetWeaponType();

	if (bReloading) OffsetState = EOffsetState::EOS_Reloading;
	else if (bIsInAir) OffsetState = EOffsetState::EOS_InAir;
	else if (bAiming) OffsetState = EOffsetState::EOS_Aiming;
	else OffsetState = EOffsetState::EOS_Hip;

	TurnInPlace();
	Lean(DeltaTime);
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

	if (Speed > 0.f || bIsInAir)
	{
		RootYawOffset = 0.f;
		TIPCharacterYaw = NimbleTerminatorCharacter->GetActorRotation().Yaw;
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		RotationCurve = 0.f;
		RotationCurveLastFrame = 0.f;
	}
	else
	{
		TIPCharacterYawLastFrame = TIPCharacterYaw;
		TIPCharacterYaw = NimbleTerminatorCharacter->GetActorRotation().Yaw;
		// Difference between CharacterYaw and CharacterYawLastFrame
		const float TIPYawDelta = TIPCharacterYaw - TIPCharacterYawLastFrame;

		// Root Yaw offset, updated and clamped to [-180, 180]
		RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - TIPYawDelta);

		// 1.0 if turning, 0.0 if not (Curve created in the turning animations)
		const float Turning = GetCurveValue(TEXT("Turning"));
		if (Turning > 0.f)
		{
			bTurningInPlace = true;
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
		else
			bTurningInPlace = false;
	}


	if (bTurningInPlace)
	{
		if (bReloading || bEquipping)
			RecoilWeight = 1.f;
		else
			RecoilWeight = 0.f;
	}
	else
	{
		if (bCrouching)
		{
			if (bReloading || bEquipping)
				RecoilWeight = 1.f;
			else
				RecoilWeight = 0.1f;
		}
		else
		{
			if (bAiming || bReloading || bEquipping)
				RecoilWeight = 1.f;
			else
				RecoilWeight = 0.5f;
		}
	}
}

void UNimbleTerminatorAnimInstance::Lean(float DeltaTime)
{
	if (NimbleTerminatorCharacter == nullptr) return;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = NimbleTerminatorCharacter->GetActorRotation();

	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.f);

	YawDelta = FMath::Clamp(Interp, -90.f, 90.f);
}
