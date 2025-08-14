// Copyright (c) 2025 Code1133. All rights reserved.

#include "Component/OCGTerrainGenerateComponent.h"

#include "OCGLevelGenerator.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Components/BoxComponent.h"
#include "Data/MapPreset.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Utils/OCGUtils.h"


UOCGTerrainGenerateComponent::UOCGTerrainGenerateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UOCGTerrainGenerateComponent::GenerateTerrainInEditor()
{
#if WITH_EDITOR
	GenerateTerrain(GetWorld());
#endif
}

void UOCGTerrainGenerateComponent::GenerateTerrain(UWorld* World)
{
#if WITH_EDITOR
	const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
	if (!LevelGenerator || !LevelGenerator->GetMapPreset())
	{
		return;
	}

	AOCGLandscapeVolume* OCGVolumeInstance = nullptr;
	if (OCGVolumeAssetSoftObjectPtr.ToSoftObjectPath().IsValid())
	{
		OCGVolumeInstance = Cast<AOCGLandscapeVolume>(OCGVolumeAssetSoftObjectPtr.Get());
		if (!IsValid(OCGVolumeInstance))
		{
			OCGVolumeInstance = Cast<AOCGLandscapeVolume>(OCGVolumeAssetSoftObjectPtr.LoadSynchronous());
		}
	}

	TArray<AOCGLandscapeVolume*> Volumes = FOCGUtils::GetAllActorsOfClass<AOCGLandscapeVolume>(World);
	
	// 삭제할 볼륨 수집
	TArray<AOCGLandscapeVolume*> ToDestroy;
	for (AOCGLandscapeVolume* Volume : Volumes)
	{
		if (Volume != OCGVolumeInstance)
		{
			ToDestroy.Add(Volume);
		}
	}
	
	// 변경(삭제 또는 생성)이 일어나면 더티 표시
	const bool bNeedsCreation = !IsValid(OCGVolumeInstance);
	if (ToDestroy.Num() > 0 || bNeedsCreation)
	{
		Modify();
		if (AActor* Owner = GetOwner())
		{
			Owner->Modify();
			(void)Owner->MarkPackageDirty();
		}
	}
	
	TArray<AOCGLandscapeVolume*> VolumesToDestroy = ToDestroy; // 복사본 생성
	for (AOCGLandscapeVolume* Vol : VolumesToDestroy)
	{
		World->EditorDestroyActor(Vol, true);
	}
	
	// 인스턴스 없으면 새로 스폰
	if (bNeedsCreation)
	{
		OCGVolumeInstance = World->SpawnActor<AOCGLandscapeVolume>();
		OCGVolumeInstance->SetIsSpatiallyLoaded(false);
		OCGVolumeInstance->Modify();
	}

	// SoftPtr 갱신
	if (IsValid(OCGVolumeInstance))
	{
		OCGVolumeAssetSoftObjectPtr = OCGVolumeInstance;
	}

	OCGVolumeInstance->SetActorLocation(LevelGenerator->GetVolumeOrigin());
	OCGVolumeInstance->GetBoxComponent()->SetBoxExtent(LevelGenerator->GetVolumeExtent());

	const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
	OCGVolumeInstance->MapPreset = MapPreset;

	if (UPCGGraph* PCGGraph = MapPreset->PCGGraph)
	{
		if (UPCGComponent* PCGComponent = OCGVolumeInstance->GetPCGComponent())
		{
			PCGComponent->SetGraph(PCGGraph);
			if (MapPreset->bAutoGenerate)
			{
				PCGComponent->Generate(true);
			}
		}
	}
#endif
}

AOCGLevelGenerator* UOCGTerrainGenerateComponent::GetLevelGenerator() const
{
	return Cast<AOCGLevelGenerator>(GetOwner());
}

