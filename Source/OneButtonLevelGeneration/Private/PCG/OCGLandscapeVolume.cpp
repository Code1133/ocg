// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/OCGLandscapeVolume.h"

#include "Landscape.h"
#include "PCGComponent.h"
#include "Components/BoxComponent.h"
#include "Conditions/MovieSceneScalabilityCondition.h"
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
	PCGComponent->bRegenerateInEditor = bEditorAutoGenerate;

}

void AOCGLandscapeVolume::AdjustVolumeToBoundsOfActor(const AActor* TargetActor)
{
	check(TargetActor);

	if (!TargetActor->IsA<ALandscape>())
	{
		return;
	}

	// GetActorBounds calculates the bounding box (FBox) that includes the actor and all its components.
	FVector Origin = TargetActor->GetActorLocation();

	const ALandscape* LandscapeActor = Cast<ALandscape>(TargetActor);
	FVector BoxExtent = LandscapeActor->GetLoadedBounds().GetExtent();

	// add offset
	Origin += FVector(BoxExtent.X , BoxExtent.Y,0);
	// Set the location of this actor (volume) to the center of the target actor's bounds
	SetActorLocation(Origin);

	// Set the BoxComponent's extent to match the target actor's bounding box extent
	BoxComponent->SetBoxExtent(BoxExtent);
}

void AOCGLandscapeVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, bEditorAutoGenerate))
	{
		PCGComponent->bRegenerateInEditor = bEditorAutoGenerate;
	}
}
