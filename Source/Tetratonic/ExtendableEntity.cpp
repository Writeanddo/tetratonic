// Fill out your copyright notice in the Description page of Project Settings.


#include "ExtendableEntity.h"

#include "PlayerCollider.h"
#include "TrackGameMode.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AExtendableEntity::AExtendableEntity()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	StartCapComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StartCap"));
	SetRootComponent(StartCapComponent);
	
	EndCapComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EndCap"));

	DynamicComponentRefs = TArray<USceneComponent*>();
}

// Called when the game starts or when spawned
void AExtendableEntity::BeginPlay()
{
	Super::BeginPlay();

	if (const ATrackGameMode* TrackGameMode = Cast<ATrackGameMode>(GetWorld()->GetAuthGameMode()))
	{
		ClockHandle = TrackGameMode->QuartzClock;

		const FRotator TargetPositionRotator = FRotator(90 * static_cast<int>(TargetPosition), 0, 0);
		TargetPositionOffset = static_cast<bool>(TargetPosition) * TargetPositionRotator.Vector() * TrackGameMode->GetPlayfieldRadius();
	}
}

void AExtendableEntity::OnConstruction(const FTransform& Transform)
{
	for (const auto& DynamicComponent : DynamicComponentRefs)
	{
		if (DynamicComponent)
		{
			DynamicComponent->DestroyComponent();
		}
	}
	DynamicComponentRefs.Empty();
	
	if (!StartCapComponent || !EndCapComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ExtendableEntity %s does not have start or end cap components, skipping construction."), *GetActorNameOrLabel());
	}
	EndCapComponent->AttachToComponent(StartCapComponent, FAttachmentTransformRules::KeepRelativeTransform);
	
	StartCapComponent->SetStaticMesh(StartCapMesh);
	EndCapComponent->SetStaticMesh(EndCapMesh);

	StartCapComponent->SetMaterial(0, StartCapMaterial);
	EndCapComponent->SetMaterial(0, EndCapMaterial);

	EndCapComponent->SetRelativeLocation(FVector(-1, 0, 0) * Speed * NumBeats);

	for (float SpannerCollisionOffset = 0.5; SpannerCollisionOffset < NumBeats; SpannerCollisionOffset += 0.5)
	{
		AddSpannerCollider(SpannerCollisionOffset);
	}

	const FRotator MovementRotator = FRotator(45 * static_cast<int>(Direction), 0, 0);
	StartCapComponent->SetRelativeRotation(MovementRotator);
}

void AExtendableEntity::AddSpannerCollider(float BeatOffset)
{
	USphereComponent* SphereComponent = NewObject<USphereComponent>(this);
	SphereComponent->AttachToComponent(StartCapComponent, FAttachmentTransformRules::KeepRelativeTransform);
	SphereComponent->SetRelativeLocation(FVector(-1, 0, 0) * Speed * BeatOffset);
	SphereComponent->SetSphereRadius(SpannerColliderRadius);
	SphereComponent->RegisterComponent();

	UPlayerCollider* PlayerCollider = NewObject<UPlayerCollider>(this);
	PlayerCollider->AttachToComponent(SphereComponent, FAttachmentTransformRules::KeepRelativeTransform);
	PlayerCollider->bResetsCombo = true;
	PlayerCollider->HealthModifier = SpannerHealthModifier;
	PlayerCollider->RegisterComponent();
	
	DynamicComponentRefs.Add(SphereComponent);
}

// Called every frame
void AExtendableEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ClockHandle)
	{
		const FQuartzTransportTimeStamp Timestamp = ClockHandle->GetCurrentTimestamp(GetWorld());
		const float BeatFraction = ClockHandle->GetBeatProgressPercent();
		
		float BeatOffset = (Timestamp.Bars - 1) * 4 + (Timestamp.Beat - TargetBeat);
		if (!UseDiscreteMotion)
		{
			const float TimestampError = FMath::Abs(Timestamp.BeatFraction - BeatFraction);
			float SelectedFraction = BeatFraction;
			
			if (TimestampError > 0.9)		// Adjust for wrap-around of beat fraction on downbeat
			{
				BeatOffset += 1;
			}
			else if (TimestampError > 0.1)	// Adjust for potential desync when unpausing
			{
				SelectedFraction = Timestamp.BeatFraction;
			}
			
			const float FractionComponent = 1 - FMath::Sqrt(1 - FMath::Pow(SelectedFraction, 2));
			BeatOffset += FractionComponent;
		}
		
		SetActorLocation(GetActorForwardVector() * Speed * BeatOffset + TargetPositionOffset);
	}
}

