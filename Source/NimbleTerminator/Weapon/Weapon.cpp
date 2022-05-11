// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Keep the Weapon upright
	if (GetItemState() == EItemState::EIS_Falling && bFalling && GetItemMesh())
	{
		const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void AWeapon::ThrowWeapon()
{
	if (GetItemMesh())
	{
		const FRotator MeshRotation(0.f, GetItemMesh()->GetComponentRotation().Yaw, 0.f);
		GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

		const FVector MeshForward(GetItemMesh()->GetForwardVector());
		const FVector MeshRight(GetItemMesh()->GetRightVector());
		
		// Direction in which we throw the weapon
		FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.f, MeshForward);

		const float RandomRotation(FMath::FRandRange(0.f, 30.f));
		ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(0.f, 0.f, 1.f));
		ImpulseDirection *= 10'000.f;
		
		GetItemMesh()->AddImpulse(ImpulseDirection);

		bFalling = true;
		GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &ThisClass::StopFalling, ThrowWeaponTime);
	}
}

void AWeapon::StopFalling()
{
	bFalling = false;
	SetItemState(EItemState::EIS_Pickup);
}
