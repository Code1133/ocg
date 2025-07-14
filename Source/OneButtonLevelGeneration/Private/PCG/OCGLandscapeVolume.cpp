// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/OCGLandscapeVolume.h"

#include "Landscape.h"
#include "PCGComponent.h"
#include "Components/BoxComponent.h"


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

#if WITH_EDITOR
	PCGComponent->bRegenerateInEditor = bEditorAutoGenerate;

	SetIsSpatiallyLoaded(false);
#endif
}

#if WITH_EDITOR
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

void AOCGLandscapeVolume::SetEditorAutoGenerate(bool bEnable)
{
	bEditorAutoGenerate = bEnable;
	PCGComponent->bRegenerateInEditor = bEnable;
}
#endif
