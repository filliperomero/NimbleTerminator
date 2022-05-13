// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorPlayerController.h"

#include "Blueprint/UserWidget.h"

ANimbleTerminatorPlayerController::ANimbleTerminatorPlayerController()
{
}

void ANimbleTerminatorPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HUDOverlayClass)
	{
		HUDOverlay = CreateWidget<UUserWidget>(this, HUDOverlayClass);
		if (HUDOverlay)
		{
			HUDOverlay->AddToViewport();
			HUDOverlay->SetVisibility(ESlateVisibility::Visible);
		}
	}
}
