// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NimbleTerminator/Weapon/AmmoType.h"
#include "NimbleTerminatorCharacter.generated.h"

#define TRACE_LENGTH 80'000.f

UENUM()
enum class ECombatState: uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_FireTimerInProgress UMETA(DisplayName = "FireTimerInProgress"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),

	ECS_MAX UMETA(DisplayName = "DefaultMax"),
};

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
	void MoveForward(float Value);
	void MoveRight(float Value);
	
	/**
	 * Called via input to turn at a given rate
	 * @param Rate  This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);
	/**
	 * Rotate controller based on mouse X movement
	 * @param Value  The input value from mouse movement
	 */
	void Turn(float Value);
	
	/**
	 * Called via input to look up/down at a given rate
	 * @param Rate  This is a normalized rate, i.e. 1.0 means 100% of desired rate
	 */
	void LookUpAtRate(float Rate);
	/**
	 * Rotate controller based on mouse Y movement
	 * @param Value  The input value from mouse movement
	 */
	void LookUp(float Value);

	// Called when the fire button is pressed
	void FireWeapon();
	void PlayFireSound();
	void PlayGunfireMontage();
	void SendBullet();

	bool GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation);

	// Set bAiming to true/false with button press
	void AimingButtonPressed();
	void AimingButtonRelease();
	void Aim();
	void StopAiming();

	void InterpFOV(float DeltaTime);

	// Set BaseTurnRate and BaseLookUpRate based on aiming
	void SetLookRates();

	void CalculateCrosshairSpread(float DeltaTime);
	
	void StartCrosshairBulletFire();
	
	UFUNCTION()
	void FinishCrosshairBulletFire();

	void FireButtonPressed();
	void FireButtonReleased();

	// Line trace for items under the crosshairs
	bool TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation);

	void TraceForItems();
	class AWeapon* SpawnDefaultWeapon();
	void EquipWeapon(AWeapon* WeaponToEquip);

	void DropWeapon();
	void SelectButtonPressed();
	void SelectButtonReleased();
	void SwapWeapon(AWeapon* WeaponToSwap);

	void InitializeAmmoMap();
	bool WeaponHasAmmo();

	void ReloadButtonPressed();
	void ReloadWeapon();
	
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	bool HasCarriedAmmo();

	// Called from animation blueprint with Grab Clip notify
	UFUNCTION(BlueprintCallable)
	void GrabClip();

	// Called from animation blueprint with Release Clip notify
	UFUNCTION(BlueprintCallable)
	void ReleaseClip();

	void CrouchButtonPressed();
	// Interps capsule Height when crouching/stading
	void InterpCapsuleHalfHeight(float DeltaTime);
	
	virtual void Jump() override;

	void PickupAmmo(class AAmmo* Ammo);	

private:
	// Camera boom positioning the camera behind the Character 
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	// Camera that follows the Character
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

	// Base turn rate, in deg/sec. Other scaling may affect final turn rate
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseTurnRate;

	// Turn Rate while not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipTurnRate = 45.f;

	// Turn Rate while aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingTurnRate = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseHipTurnRate = 1.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseAimingTurnRate = 0.6f;

	// Base look up/down rate, in deg/sec. Other scaling may affect final turn rate
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float BaseLookUpRate;

	// Look up Rate while not aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float HipLookUpRate = 45.f;

	// Look up rate while aiming
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	float AimingLoopUpRate = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseLookUpRate = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"), meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float MouseAimingLookUpRate = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class USoundCue* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* MuzzleFlash;

	// Montage for fire the weapon
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	class UAnimMontage* HipFireMontage;

	// Particles spawned upon bullet impact
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* ImpactParticles;

	// Smoke trail for bullets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* BeamParticles;

	/**
	 * Aiming
	 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	bool bAiming = false;

	bool bAimingButtonPressed = false;

	float CameraDefaultFOV = 0.f;
	float CameraCurrentFOV = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	float ZoomInterpSpeed = 20.f;

	UPROPERTY(EditAnywhere, Category = Combat)
	float CameraZoomedFOV = 25.f;

	/**
	 * Crosshair
	 */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairSpreadMultiplier = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairVelocityFactor = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairInAirFactor = 0.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairAimFactor = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Crosshairs, meta = (AllowPrivateAccess = "true"))
	float CrosshairShootingFactor = 0.f;

	float ShootTimeDuration = 0.05f;
	bool bFiringBullet = false;

	FTimerHandle CrosshairShootTimer;

	/**
	 * Shooting
	 */
	bool bFireButtonPressed = false;
	bool bCanFire = true;
	UPROPERTY(EditAnywhere, Category = Combat)
	float AutomaticFireRate = 0.15f;

	FTimerHandle FireTimer;

	void StartFireTimer();
	void FireTimerFinished();

	/**
	 * Trace Items
	 */
	// True if we should trace for every frame for items
	bool bShouldTraceForItems = false;
	int16 OverlappedItemCount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	class AItem* TraceHitItemLastFrame;

	// The item currently hit by our trace in TraceForItems (could be nullptr)
	UPROPERTY()
	AItem* TraceHitItem;

	/**
	 * Weapon
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	AWeapon* EquippedWeapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AWeapon> DefaultWeaponClass;

	// Distance outward from the camera for the interp destination
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpDistance = 250.f;

	// Distance outward from the camera for the interp destionation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Items, meta = (AllowPrivateAccess = "true"))
	float CameraInterpElevation = 65.f;

	/**
	 * Ammo
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	TMap<EAmmoType, int32> AmmoMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	int32 Starting9mmAmmo = 85;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	int32 StartingARAmmo = 120;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Combat, meta = (AllowPrivateAccess = "true"))
	UAnimMontage* ReloadMontage;

	// Transform of the clip when we first grab the clip during reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	FTransform ClipTransform;

	// Scene component to attach to the Character's hand during reloading
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Combat, meta = (AllowPrivateAccess = "true"))
	USceneComponent* HandSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bCrouching = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float StandingCapsuleHalfHeight = 88.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchingCapsuleHalfHeight = 44.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseMovementSpeed = 650.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchMovementSpeed = 300.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float BaseGroundFriction = 2.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float CrouchingGroundFriction = 100.f;
	
public:

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool GetAiming() const { return bAiming; }

	UFUNCTION(BlueprintCallable)
	float GetCrosshairSpreadMultiplier() const;

	FORCEINLINE int16 GetOverlappedItemCount() const { return OverlappedItemCount; }

	void IncrementOverlappedItemCount(int16 Amount);

	FVector GetCameraInterpLocation();

	void GetPickupItem(AItem* Item);

	FORCEINLINE ECombatState GetCombatState() const { return CombatState; }
	FORCEINLINE bool IsCrouching() const { return bCrouching; }

};
