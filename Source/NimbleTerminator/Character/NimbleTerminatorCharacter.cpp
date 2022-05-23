// Fill out your copyright notice in the Description page of Project Settings.


#include "NimbleTerminatorCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NimbleTerminator/Weapon/Item.h"
#include "NimbleTerminator/Weapon/Weapon.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "Components/WidgetComponent.h"
#include "NimbleTerminator/Weapon/Ammo.h"

ANimbleTerminatorCharacter::ANimbleTerminatorCharacter() :
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create a Camera Boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 240.f;
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->SocketOffset = FVector(0.f, 35.f, 80.f);

	// Create a Follow Camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false; // FollowCamera does not rotate relative to arm

	// Don't rotate when the controller rotates. Let the controller only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ... at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	HandSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));
}

void ANimbleTerminatorCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	EquipWeapon(SpawnDefaultWeapon());
	InitializeAmmoMap();

	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
}

void ANimbleTerminatorCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	InterpFOV(DeltaTime);
	SetLookRates();
	CalculateCrosshairSpread(DeltaTime);
	TraceForItems();
	// Interpolate the capsule half height based on crouching/standing
	// InterpCapsuleHalfHeight(DeltaTime);
}

void ANimbleTerminatorCharacter::InterpFOV(float DeltaTime)
{
	if (bAiming)
	{
		// Interpolate to zoomed FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	}
	else
	{
		// Interpolate to Default FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	}

	if (GetFollowCamera())
	{
		GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
	}
}

void ANimbleTerminatorCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	const float TargetCapsuleHalfHeight = bCrouching ? CrouchingCapsuleHalfHeight : StandingCapsuleHalfHeight;
	const float InterpHalfHeight = FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
	                                                TargetCapsuleHalfHeight, DeltaTime, 20.f);

	// Negative value if crouching and positive value if standing
	const float DeltaCapsuleHalfHeight = InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector MeshOffset(0.f, 0.f, -DeltaCapsuleHalfHeight);

	GetMesh()->AddLocalOffset(MeshOffset);
	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight, true);
}

void ANimbleTerminatorCharacter::SetLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLoopUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = AimingLoopUpRate;
	}
}

void ANimbleTerminatorCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	const float MaxWalkSpeed = GetCharacterMovement() ? GetCharacterMovement()->MaxWalkSpeed : 600.f;
	const FVector2D WalkSpeedRange(0.f, MaxWalkSpeed);
	const FVector2D VelocityMultiplierRange(0.f, 1.f);
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;

	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange,
	                                                            Velocity.Size());

	CrosshairInAirFactor = GetCharacterMovement()->IsFalling()
		                       ? FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f)
		                       : FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);

	CrosshairAimFactor = bAiming
		                     ? FMath::FInterpTo(CrosshairAimFactor, 0.4f, DeltaTime, 30.f)
		                     : FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);

	CrosshairShootingFactor = bFiringBullet
		                          ? FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.f)
		                          : FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 60.f);

	CrosshairSpreadMultiplier = 0.5f
		+ CrosshairVelocityFactor
		+ CrosshairInAirFactor
		- CrosshairAimFactor
		+ CrosshairShootingFactor;
}

void ANimbleTerminatorCharacter::TraceForItems()
{
	if (bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);

		if (ItemTraceResult.bBlockingHit)
		{
			TraceHitItem = Cast<AItem>(ItemTraceResult.GetActor());
			if (TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				TraceHitItem->GetPickupWidget()->SetVisibility(true);
			}

			// We are hitting a different AItem this frame from last frame or AItem is null
			if (TraceHitItemLastFrame && TraceHitItemLastFrame != TraceHitItem)
			{
				TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
			}

			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if (TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility(false);
		TraceHitItemLastFrame = nullptr;
		TraceHitItem = nullptr;
	}
}

float ANimbleTerminatorCharacter::GetCrosshairSpreadMultiplier() const
{
	return CrosshairSpreadMultiplier;
}

void ANimbleTerminatorCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ThisClass::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ThisClass::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &ThisClass::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ThisClass::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &ThisClass::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ThisClass::LookUp);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ThisClass::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &ThisClass::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &ThisClass::FireButtonReleased);
	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &ThisClass::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &ThisClass::AimingButtonRelease);
	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &ThisClass::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &ThisClass::SelectButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ThisClass::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ThisClass::CrouchButtonPressed);
}

void ANimbleTerminatorCharacter::MoveForward(float Value)
{
	if (Controller == nullptr || Value == 0.f)
	{
		return;
	}

	const FRotator Rotation{Controller->GetControlRotation()};
	const FRotator YawRotation{0.f, Rotation.Yaw, 0.f};
	const FVector Direction{FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X)};

	AddMovementInput(Direction, Value);
}

void ANimbleTerminatorCharacter::MoveRight(float Value)
{
	if (Controller == nullptr || Value == 0.f)
	{
		return;
	}

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));

	AddMovementInput(Direction, Value);
}

void ANimbleTerminatorCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANimbleTerminatorCharacter::Turn(float Value)
{
	float TurnScaleFactor;
	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(Value * TurnScaleFactor);
}

void ANimbleTerminatorCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ANimbleTerminatorCharacter::LookUp(float Value)
{
	float LookUpScaleFactor;
	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseLookUpRate;
	}

	AddControllerPitchInput(Value * LookUpScaleFactor);
}

void ANimbleTerminatorCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void ANimbleTerminatorCharacter::ReloadWeapon()
{
	if (EquippedWeapon == nullptr
		|| CombatState != ECombatState::ECS_Unoccupied
		|| !HasCarriedAmmo()
		|| EquippedWeapon->IsClipFull())
	{
		return;
	}

	if (bAiming)
	{
		StopAiming();
	}

	if (GetMesh())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if (AnimInstance && ReloadMontage)
		{
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
			CombatState = ECombatState::ECS_Reloading;
		}
	}
}

void ANimbleTerminatorCharacter::FinishReloading()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if (bAimingButtonPressed)
	{
		Aim();
	}

	if (EquippedWeapon == nullptr)
	{
		return;
	}
	const auto AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType))
	{
		int32 CarriedAmmo = AmmoMap[AmmoType];
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity() - EquippedWeapon->GetAmmo();

		if (MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
		}
		else
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
		}

		AmmoMap.Add(AmmoType, CarriedAmmo);
	}
}

bool ANimbleTerminatorCharacter::HasCarriedAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}

	const auto AmmoType = EquippedWeapon->GetAmmoType();

	if (AmmoMap.Contains(AmmoType))
	{
		return AmmoMap[AmmoType] > 0;
	}

	return false;
}

void ANimbleTerminatorCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}

void ANimbleTerminatorCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}

void ANimbleTerminatorCharacter::StartFireTimer()
{
	CombatState = ECombatState::ECS_FireTimerInProgress;

	GetWorldTimerManager().SetTimer(FireTimer, this, &ThisClass::FireTimerFinished, AutomaticFireRate);
}

void ANimbleTerminatorCharacter::FireTimerFinished()
{
	CombatState = ECombatState::ECS_Unoccupied;
	if (WeaponHasAmmo())
	{
		if (bFireButtonPressed)
		{
			FireWeapon();
		}
	}
	else
	{
		ReloadWeapon();
	}
}

void ANimbleTerminatorCharacter::FireWeapon()
{
	if (EquippedWeapon == nullptr || CombatState != ECombatState::ECS_Unoccupied || !WeaponHasAmmo())
	{
		return;
	}

	PlayFireSound();
	SendBullet();
	PlayGunfireMontage();
	StartCrosshairBulletFire();

	EquippedWeapon->DecrementAmmo();
	StartFireTimer();
}

bool ANimbleTerminatorCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if (bCrosshairHit)
	{
		OutBeamLocation = CrosshairHitResult.Location;
	}

	// Perform a second trace, this time from the gun barrel
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart(MuzzleSocketLocation);
	const FVector StartToEnd(OutBeamLocation - MuzzleSocketLocation);
	// Increase by 25%
	const FVector WeaponTraceEnd(MuzzleSocketLocation + StartToEnd * 1.25f);

	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECC_Visibility);

	// Object between barrel and BeamEndPOint
	if (WeaponTraceHit.bBlockingHit)
	{
		OutBeamLocation = WeaponTraceHit.ImpactPoint;
		return true;
	}

	return false;
}

void ANimbleTerminatorCharacter::PlayFireSound()
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
}

void ANimbleTerminatorCharacter::PlayGunfireMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void ANimbleTerminatorCharacter::SendBullet()
{
	const USkeletalMeshSocket* BarrelSocket = EquippedWeapon->GetItemMesh()->GetSocketByName(FName("BarrelSocket"));

	if (BarrelSocket)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(EquippedWeapon->GetItemMesh());

		UWorld* World = GetWorld();
		if (World)
		{
			if (MuzzleFlash)
			{
				UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
			}

			FVector BeamEnd;
			bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);

			if (bBeamEnd)
			{
				if (ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(World, ImpactParticles, BeamEnd);
				}

				if (BeamParticles)
				{
					UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
						World, BeamParticles, SocketTransform);

					if (Beam)
					{
						Beam->SetVectorParameter(FName("Target"), BeamEnd);
					}
				}
			}
		}
	}
}

void ANimbleTerminatorCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &ThisClass::FinishCrosshairBulletFire,
	                                ShootTimeDuration);
}

void ANimbleTerminatorCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void ANimbleTerminatorCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if (CombatState != ECombatState::ECS_Reloading)
	{
		Aim();
	}
}

void ANimbleTerminatorCharacter::AimingButtonRelease()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void ANimbleTerminatorCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}

void ANimbleTerminatorCharacter::StopAiming()
{
	bAiming = false;
	if (!bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
}

bool ANimbleTerminatorCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	// Get current size of the viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	// Get screen space location of crosshairs
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	// Enable if want to offset by 50 (must change in the HUD blueprint)
	// CrosshairLocation.Y -= 50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	// Get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection
	);

	// Was deprojection succesfull
	if (bScreenToWorld)
	{
		const FVector Start(CrosshairWorldPosition);
		const FVector End(Start + CrosshairWorldDirection * TRACE_LENGTH);
		OutHitLocation = End;

		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECC_Visibility);

		if (OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;
		}
	}

	return false;
}

void ANimbleTerminatorCharacter::IncrementOverlappedItemCount(int16 Amount)
{
	if (OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
		return;
	}

	OverlappedItemCount += Amount;
	bShouldTraceForItems = true;
}

AWeapon* ANimbleTerminatorCharacter::SpawnDefaultWeapon()
{
	if (DefaultWeaponClass)
	{
		AWeapon* DefaultWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);

		return DefaultWeapon;
	}

	return nullptr;
}

void ANimbleTerminatorCharacter::EquipWeapon(AWeapon* WeaponToEquip)
{
	if (WeaponToEquip == nullptr)
	{
		return;
	}

	const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));

	if (HandSocket)
	{
		HandSocket->AttachActor(WeaponToEquip, GetMesh());
	}

	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
}

void ANimbleTerminatorCharacter::DropWeapon()
{
	if (EquippedWeapon == nullptr)
	{
		return;
	}

	if (EquippedWeapon->GetItemMesh())
	{
		const FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);
		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();

		EquippedWeapon = nullptr;
	}
}

void ANimbleTerminatorCharacter::SelectButtonPressed()
{
	if (TraceHitItem)
	{
		// const auto TraceHitWeapon = Cast<AWeapon>(TraceHitItem);
		// SwapWeapon(TraceHitWeapon);
		TraceHitItem->StartItemCurve(this);

		if (TraceHitItem->GetPickupSound())
		{
			UGameplayStatics::PlaySound2D(this, TraceHitItem->GetPickupSound());
		}
	}
	else
	{
		DropWeapon();
	}
}

void ANimbleTerminatorCharacter::SelectButtonReleased()
{
}

void ANimbleTerminatorCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	DropWeapon();
	EquipWeapon(WeaponToSwap);
	TraceHitItem = nullptr;
	TraceHitItemLastFrame = nullptr;
}

FVector ANimbleTerminatorCharacter::GetCameraInterpLocation()
{
	if (FollowCamera == nullptr)
	{
		return FVector(0.f, 0.f, 0.f);
	}

	const FVector CameraWorldLocation(FollowCamera->GetComponentLocation());
	const FVector CameraForward(FollowCamera->GetForwardVector());

	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(0.f, 0.f, CameraInterpElevation);
}

void ANimbleTerminatorCharacter::GetPickupItem(AItem* Item)
{
	if (Item->GetEquipSound())
	{
		UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	}

	auto Weapon = Cast<AWeapon>(Item);

	if (Weapon)
	{
		SwapWeapon(Weapon);
	}

	auto Ammo = Cast<AAmmo>(Item);

	if (Ammo)
	{
		PickupAmmo(Ammo);
	}
}

void ANimbleTerminatorCharacter::PickupAmmo(AAmmo* Ammo)
{
	if (AmmoMap.Find(Ammo->GetAmmoType()))
	{
		int32 AmmoCount = AmmoMap[Ammo->GetAmmoType()];
		AmmoCount += Ammo->GetItemCount();
		AmmoMap.Add(Ammo->GetAmmoType(), AmmoCount);
	}

	if (EquippedWeapon->GetAmmoType() == Ammo->GetAmmoType() && EquippedWeapon->GetAmmo() == 0)
	{
		ReloadWeapon();
	}

	Ammo->Destroy();
}

void ANimbleTerminatorCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool ANimbleTerminatorCharacter::WeaponHasAmmo()
{
	if (EquippedWeapon == nullptr)
	{
		return false;
	}

	return EquippedWeapon->GetAmmo() > 0;
}

void ANimbleTerminatorCharacter::GrabClip()
{
	if (EquippedWeapon == nullptr || HandSceneComponent == nullptr || EquippedWeapon->GetItemMesh() == nullptr)
	{
		return;
	}

	const int32 ClipBoneIndex = EquippedWeapon->GetItemMesh()->GetBoneIndex(EquippedWeapon->GetClipBoneName());
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	const FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneComponent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	HandSceneComponent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

void ANimbleTerminatorCharacter::ReleaseClip()
{
	EquippedWeapon->SetMovingClip(false);
}

void ANimbleTerminatorCharacter::CrouchButtonPressed()
{
	if (!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}

	if (bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void ANimbleTerminatorCharacter::Jump()
{
	if (bCrouching)
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else
	{
		Super::Jump();
	}
}
