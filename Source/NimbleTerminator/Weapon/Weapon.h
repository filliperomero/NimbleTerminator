// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "WeaponType.h"
#include "NimbleTerminator/Weapon/AmmoType.h"
#include "Engine/DataTable.h"
#include "Weapon.generated.h"

class UParticleSystem;

USTRUCT(BlueprintType)
struct FWeaponDataTable : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EAmmoType AmmoType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WeaponAmmo;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MagazineCapacity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutomatic;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USoundCue* PickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* EquipSound;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USkeletalMesh* ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* InventoryIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* AmmoIcon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaterialIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UMaterialInstance* MaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ClipBoneName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ReloadMontageSection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimBP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairsMiddle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairsBottom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AutoFireRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	USoundCue* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName BoneToHide;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HeadShotDamage;
};

/**
 * 
 */
UCLASS()
class NIMBLETERMINATOR_API AWeapon : public AItem
{
	GENERATED_BODY()

public:
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	
protected:
	void StopFalling();

	/** Override's */

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	void UpdateSlideDisplacement();
	void FinishMovingSlide();

private:
	FTimerHandle ThrowWeaponTimer;
	float ThrowWeaponTime = 0.7f;
	bool bFalling = false;

	// True for auto gunfire
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bAutomatic { true };

	/**
	 * Ammo
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 Ammo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	int32 MagazineCapacity = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EAmmoType AmmoType = EAmmoType::EAT_9mm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	EWeaponType WeaponType = EWeaponType::EWT_SubmachineGun;
					
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName ReloadMontageSection = FName(TEXT("Reload SMG"));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	bool bMovingClip = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	FName ClipBoneName = FName(TEXT("smg_clip"));

	// Data Table for Weapon Properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UDamageType* WeaponDataTable;

	int32 PreviousMaterialIndex;

	/** Crosshairs */

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairsMiddle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairsLeft;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairsRight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairsBottom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UTexture2D* CrosshairsTop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	float AutoFireRate;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	UParticleSystem* MuzzleFlash;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	USoundCue* FireSound;

	// Name of the bone to hide on the weapon mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = DataTable, meta = (AllowPrivateAccess = "true"))
	FName BoneToHide;

	/** Slide Displacement */

	// Amount that the slide is pushed back during pistol fire
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float SlideDisplacement { 0.f };

	// Amount that the pistol will rotate during pistol fire
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float RecoilRotation { 0.f };

	// Max distance for the Slide on the pistol
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float MaxSlideDisplacement { 4.f };

	// Max Rotation for pistol recoil
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float MaxRecoilRotation { 20.f };

	// Curve for the Slide Displacement
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	UCurveFloat* SlideDisplacementCurve;

	// Timer Handle for updating SlideDisplacement
	FTimerHandle SlideTimer;

	// Time for displacing the slide during pistol fire
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	float SlideDisplacementTime { 0.2f };

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Pistol, meta = (AllowPrivateAccess = "true"))
	bool bMovingSlide { false };

	/** Damage */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float Damage { 20.f };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Properties", meta = (AllowPrivateAccess = "true"))
	float HeadShotDamage { 40.f };

public:
	void ThrowWeapon();
	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagazineCapacity() const { return MagazineCapacity; }
	FORCEINLINE bool IsWeaponAutomatic() const { return bAutomatic; }
	void DecrementAmmo();
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	FORCEINLINE EAmmoType GetAmmoType() const { return AmmoType; }
	FORCEINLINE FName GetReloadMontageSection() const { return ReloadMontageSection; }
	FORCEINLINE void SetReloadMontageSection(const FName MontageSection) { ReloadMontageSection = MontageSection; }
	FORCEINLINE FName GetClipBoneName() const { return ClipBoneName; }
	FORCEINLINE void SetClipBoneName(const FName ClipName) { ClipBoneName = ClipName; }
	FORCEINLINE void SetMovingClip(const bool Move) { bMovingClip = Move; }
	FORCEINLINE float GetAutoFireRate() const { return AutoFireRate; }
	FORCEINLINE UParticleSystem* GetMuzzleFlash() const { return MuzzleFlash; }
	FORCEINLINE USoundCue* GetFireSound() const { return FireSound; }
	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }

	void ReloadAmmo(int32 Amount);
	bool IsClipFull() const;
	void StartSlideTimer();
};
