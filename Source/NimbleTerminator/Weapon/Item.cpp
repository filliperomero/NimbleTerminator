// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NimbleTerminator/Character/NimbleTerminatorCharacter.h"
#include "Sound/SoundCue.h"

AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(RootComponent);
	AreaSphere->SetSphereRadius(150.f);
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	if (PickupWidget)
		PickupWidget->SetVisibility(false);

	if (AreaSphere)
	{
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnSphereOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnSphereEndOverlap);
	}

	SetActiveStars();

	SetItemProperties(ItemState);
	// Set custom depth to disable
	InitializeCustomDepth();
}

void AItem::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		if (ItemMesh)
			ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);

		EnableGlowMaterial();
	}
}

void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);
}

void AItem::ItemInterp(float DeltaTime)
{
	if (Character == nullptr || ItemZCurve == nullptr || !bInterping) return;

	// Elapsed time since we started ItemInterpTimer
	const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
	// Get curve value corresponding to ElapsedTime
	const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

	FVector ItemLocation = ItemInterpStartLocation;
	
	// const FVector CameraInterpLocation = Character->GetCameraInterpLocation();
	const FVector CameraInterpLocation = GetInterpLocation();
	
	// Vector from Item to  Camera InterpLocation, X and Y are zeroed out
	const FVector ItemToCamera = FVector(0.f, 0.f, (CameraInterpLocation - ItemLocation).Z);
	// Scale factor to multiply with the curveValue
	const float DeltaZ = ItemToCamera.Size();

	const FVector CurrentLocation(GetActorLocation());
	// Interpolated X and Y value
	const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 30.f);
	const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.f);

	ItemLocation.X = InterpXValue;
	ItemLocation.Y = InterpYValue;
	ItemLocation.Z += CurveValue * DeltaZ;
	SetActorLocation(ItemLocation, true, nullptr, ETeleportType::TeleportPhysics);

	if (Character->GetFollowCamera())
	{
		const FRotator CameraRotation = Character->GetFollowCamera()->GetComponentRotation();
		const FRotator ItemRotation(0.f, CameraRotation.Yaw + InterpInitialYawOffset, 0.f);

		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);
	}

	if (ItemScaleCurve)
	{
		const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
		SetActorScale3D(FVector(ScaleCurveValue));
	}
}

FVector AItem::GetInterpLocation()
{
	if (Character == nullptr) return FVector(0.f);

	switch (ItemType)
	{
	case EItemType::EIT_Ammo:
		return Character->GetInterpLocation(InterpLocIndex).SceneComponent->GetComponentLocation();
	case EItemType::EIT_Weapon:
		return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();
	default:
		return FVector(0.f);
	}
}

void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                            UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor)
	{
		ANimbleTerminatorCharacter* NimbleTerminatorCharacter = Cast<ANimbleTerminatorCharacter>(OtherActor);
		if (NimbleTerminatorCharacter)
			NimbleTerminatorCharacter->IncrementOverlappedItemCount(1);
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (OtherActor)
	{
		ANimbleTerminatorCharacter* NimbleTerminatorCharacter = Cast<ANimbleTerminatorCharacter>(OtherActor);
		if (NimbleTerminatorCharacter)
			NimbleTerminatorCharacter->IncrementOverlappedItemCount(-1);
	}
}

void AItem::SetActiveStars()
{
	// index 0 will not be used in this case
	for (int32 i = 0; i <= 5; i++)
		ActiveStars.Add(false);

	switch (ItemRarity)
	{
	case EItemRarity::EIR_Legendary:
		ActiveStars[5] = true;
	case EItemRarity::EIR_Rare:
		ActiveStars[4] = true;
	case EItemRarity::EIR_Uncommon:
		ActiveStars[3] = true;
	case EItemRarity::EIR_Common:
		ActiveStars[2] = true;
	case EItemRarity::EIR_Damaged:
		ActiveStars[1] = true;
		break;
	default:
		break;
	}
}

void AItem::SetItemProperties(const EItemState State)
{
	switch (State)
	{
	case EItemState::EIS_Pickup:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	case EItemState::EIS_Equipped:
		PickupWidget->SetVisibility(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_Falling:
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetEnableGravity(true);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	case EItemState::EIS_EquipInterping:
		PickupWidget->SetVisibility(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	default:
		break;
	}
}

void AItem::SetItemState(const EItemState State)
{
	ItemState = State;
	SetItemProperties(State);
}

void AItem::StartItemCurve(ANimbleTerminatorCharacter* Char)
{
	Character = Char;

	InterpLocIndex = Character->GetInterpLocationIndex();
	Character->IncrementInterpLocItemCount(InterpLocIndex, 1);
	
	PlayPickupSound();
	
	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);
	
	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &ThisClass::FinishInterping, ZCurveTime);

	if (Character && Character->GetFollowCamera())
	{
		const float CameraRotationYaw = Character->GetFollowCamera()->GetComponentRotation().Yaw;
		const float ItemRotationYaw = GetActorRotation().Yaw;

		InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;
	}

	bCanChangeCustomDepth = false;
}

void AItem::PlayPickupSound()
{
	if (Character == nullptr || PickupSound == nullptr) return;

	if (Character->ShouldPlayPickupSound())
	{
		Character->StartPickupSoundTimer();
		UGameplayStatics::PlaySound2D(this, PickupSound);
	}
}

void AItem::PlayEquipSound()
{
	if (Character == nullptr || EquipSound == nullptr) return;

	if (Character->ShouldPlayEquipSound())
	{
		Character->StartEquipSoundTimer();
		UGameplayStatics::PlaySound2D(this, EquipSound);
	}
}

void AItem::EnableCustomDepth()
{
	if (!bCanChangeCustomDepth) return;
	
	if (ItemMesh)
		ItemMesh->SetRenderCustomDepth(true);
}

void AItem::DisableCustomDepth()
{
	if (!bCanChangeCustomDepth) return;
	
	if (ItemMesh)
		ItemMesh->SetRenderCustomDepth(false);
}

void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
}

void AItem::EnableGlowMaterial()
{
	if (DynamicMaterialInstance)
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
}

void AItem::DisableGlowMaterial()
{
	if (DynamicMaterialInstance)
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
}

void AItem::FinishInterping()
{
	bInterping = false;
	bCanChangeCustomDepth = true;
	
	if (Character == nullptr) return;
	
	Character->GetPickupItem(this);
	Character->IncrementInterpLocItemCount(InterpLocIndex, -1);
	// Set scale back to normal
	SetActorScale3D(FVector(1.f));
	DisableGlowMaterial();
	DisableCustomDepth();
}
