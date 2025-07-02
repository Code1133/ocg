// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/OCGLandscapeVolume.h"

#include "PCGComponent.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"


AOCGLandscapeVolume::AOCGLandscapeVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetCollisionProfileName(TEXT("NoCollision"));
	BoxComponent->SetGenerateOverlapEvents(false);
	BoxComponent->InitBoxExtent(FVector{2500.0f, 2500.0f, 1000.0f});
	RootComponent = BoxComponent;

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
	PCGComponent->SetIsPartitioned(true);
}

void AOCGLandscapeVolume::AdjustVolumeToBoundsOfActor(const AActor* TargetActor)
{
	check(TargetActor);

	// GetActorBounds calculates the bounding box (FBox) that includes the actor and all its components.
	FVector Origin;
	FVector BoxExtent;
	UKismetSystemLibrary::GetActorBounds(TargetActor, Origin, BoxExtent);

	// Set the location of this actor (volume) to the center of the target actor's bounds
	SetActorLocation(Origin);

	// Set the BoxComponent's extent to match the target actor's bounding box extent
	BoxComponent->SetBoxExtent(BoxExtent);
}
