// Copyright (c) 2025 Code1133. All rights reserved.


#include "OCGTerrainVolume.h"

#include "Components/BoxComponent.h"
#include "PCGComponent.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values
AOCGTerrainVolume::AOCGTerrainVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 일단 CPP로 작성된 Class로 지정
	static ConstructorHelpers::FObjectFinder<UOCGHierarchyDataAsset> DataAssetFinder(TEXT("/Script/OneButtonLevelGeneration.OCGHierarchyDataAsset'/OneButtonLevelGeneration/DataAsset/DA_Test_LandscapeHierarchyDataAsset.DA_Test_LandscapeHierarchyDataAsset'"));

	// 2. 에셋을 성공적으로 찾았는지 확인합니다.
	// 경로가 잘못되거나 에셋이 존재하지 않으면 실패할 수 있습니다.
	if (DataAssetFinder.Succeeded())
	{
		// 3. 찾은 에셋을 TObjectPtr 멤버 변수에 직접 할당합니다.
		// FObjectFinder의 .Object 멤버는 TObjectPtr 타입이므로 바로 대입할 수 있습니다[1][6].
		DataAsset = DataAssetFinder.Object;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("기본 데이터 에셋을 찾을 수 없습니다: /Game/Path/To/YourDataAsset"));
	}
	
	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
}

// Called when the game starts or when spawned
void AOCGTerrainVolume::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOCGTerrainVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AOCGTerrainVolume::AdjustVolumeToBoundsOfActor(const AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("TargetActor가 유효하지 않습니다."));
		return;
	}

	if (!IsValid(BoxComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("이 액터에 유효한 BoundingBox Component가 없습니다."));
		return;
	}

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

	UE_LOG(LogTemp, Log, TEXT("%s를 감싸도록 볼륨 위치와 크기 조정 완료."), *TargetActor->GetName());
}

