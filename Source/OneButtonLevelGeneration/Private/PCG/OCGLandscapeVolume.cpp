// Copyright (c) 2025 Code1133. All rights reserved.

#include "PCG/OCGLandscapeVolume.h"

#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGSubsystem.h"
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
	PCGComponent->SetIsPartitioned(false);

#if WITH_EDITOR
	PCGComponent->bForceGenerateOnBPAddedToWorld = true;
#endif

	// if (UPCGSubsystem* Subsystem = GetSubsystem())
	// {
	// 	if (bGenerated)
	// 	{
	// 		CleanupLocalImmediate(/*bRemoveComponents=*/true);
	// 	}
	//
	// 	// Update the component on the subsystem
	// 	bIsComponentPartitioned = bIsNowPartitioned;
	// 	Subsystem->RegisterOrUpdatePCGComponent(this, bDoActorMapping);
	// }
}

void AOCGLandscapeVolume::AdjustVolumeToBoundsOfActor(const AActor* TargetActor)
{
	check(TargetActor);

	// 1. 대상 액터의 전체 경계를 월드 공간에서 계산합니다.
	// GetActorBounds는 액터와 그 모든 컴포넌트를 포함하는 경계(FBox)를 계산해줍니다.
	FVector Origin;
	FVector BoxExtent;
	UKismetSystemLibrary::GetActorBounds(TargetActor, Origin, BoxExtent);

	// GetActorBounds가 반환하는 Origin은 경계의 중심점(Center)입니다.
	// BoxExtent는 중심점에서 각 면까지의 거리(크기의 절반)입니다.

	// 2. 이 액터(볼륨)의 위치를 대상 액터 경계의 중심점으로 설정합니다.
	// 모든 액터는 트랜스폼을 가지고 있으므로 SetActorLocation을 사용합니다.
	SetActorLocation(Origin);

	// 3. 이 액터의 BoxComponent의 Extent를 대상 액터 경계의 Extent와 동일하게 설정합니다.
	// BoxComponent의 크기를 직접 조절합니다.
	BoxComponent->SetBoxExtent(BoxExtent);
}
