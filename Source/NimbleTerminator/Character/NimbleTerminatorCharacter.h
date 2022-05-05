// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NimbleTerminatorCharacter.generated.h"

UCLASS()
class NIMBLETERMINATOR_API ANimbleTerminatorCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ANimbleTerminatorCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
protected:
	virtual void BeginPlay() override;

public:	

};
