// Copyright (c) 2025 Code1133. All rights reserved.

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

	if (!OCGVolumeInstance)
	{
		OCGVolumeInstance = World->SpawnActor<AOCGLandscapeVolume>();
	}

	if (const ALandscape* Landscape = LevelGenerator->GetLandscape())
	{
		OCGVolumeInstance->AdjustVolumeToBoundsOfActor(Landscape);
	}

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

