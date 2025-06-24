// Fill out your copyright notice in the Description page of Project Settings.

#include "Component/OCGTerrainGenerateComponent.h"

#include "Landscape.h"
#include "OCGLevelGenerator.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Data/MapPreset.h"
#include "PCG/OCGLandscapeVolume.h"


UOCGTerrainGenerateComponent::UOCGTerrainGenerateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	TargetVolumeClass = AOCGLandscapeVolume::StaticClass();
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

	const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

	if (TargetTerrainVolume)
	{
		TargetTerrainVolume->Destroy();
	}

	if (TargetVolumeClass)
	{
		TargetTerrainVolume = World->SpawnActor<AOCGLandscapeVolume>(TargetVolumeClass);
	}

	if (TargetTerrainVolume)
	{
		if (const ALandscape* Landscape = LevelGenerator->GetLandscape())
		{
			TargetTerrainVolume->AdjustVolumeToBoundsOfActor(Landscape);
		}

		TargetTerrainVolume->DataAsset = MapPreset->PCGHierarchyData;

		if (UPCGGraph* PCGGraph = MapPreset->PCGGraph)
		{
			if (UPCGComponent* PCGComponent = TargetTerrainVolume->GetPCGComponent())
			{
				PCGComponent->SetGraph(PCGGraph);
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

