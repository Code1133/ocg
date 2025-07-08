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
	PCGComponent->bRegenerateInEditor = bEditorAutoGenerate;
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

void AOCGLandscapeVolume::AdjustVolumeToBoundsOfComponent(const UPrimitiveComponent* Comp)
{
	// 방법1: 컴포넌트의 Bounds 프로퍼티 사용
	const FBoxSphereBounds B = Comp->Bounds;
	SetActorLocation(B.Origin);
	BoxComponent->SetBoxExtent(B.BoxExtent);
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
