// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthPickup.h"

#include "NimbleTerminator/Character/NimbleTerminatorCharacter.h"

AHealthPickup::AHealthPickup()
{
}

void AHealthPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	auto Character = Cast<ANimbleTerminatorCharacter>(OtherActor);

	if (Character)
	{
		// TODO: Refactor this for a Buff Component inside Character
		Character->Heal(HealAmount);
	}

	Destroy();
}
