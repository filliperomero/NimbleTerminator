// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NimbleTerminatorPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class NIMBLETERMINATOR_API ANimbleTerminatorPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANimbleTerminatorPlayerController();

protected:
	virtual void BeginPlay() override;

private:
	// Reference to the Overall HUD Overlay Blueprint Class
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Widgets, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class UUserWidget> HUDOverlayClass;

	// Variable to hold the HUD overlay widget after creating it
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Widgets, meta = (AllowPrivateAccess = "true"))
	UUserWidget* HUDOverlay;

public:
};
