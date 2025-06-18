// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/OCGLandscapeVolume.h"
#include "PCGComponent.h"
#include "Components/BoxComponent.h"


AOCGLandscapeVolume::AOCGLandscapeVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxComponent->SetCollisionProfileName(TEXT("NoCollision"));
	BoxComponent->SetGenerateOverlapEvents(false);
	RootComponent = BoxComponent;

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
}
