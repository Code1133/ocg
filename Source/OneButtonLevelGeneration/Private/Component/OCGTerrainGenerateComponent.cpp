// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/OCGTerrainGenerateComponent.h"

#include "Landscape.h"
#include "OCGLevelGenerator.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Data/MapPreset.h"
#include "PCG/OCGLandscapeVolume.h"


// Sets default values for this component's properties
UOCGTerrainGenerateComponent::UOCGTerrainGenerateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	TargetVolumeClass = AOCGLandscapeVolume::StaticClass();
}


// Called when the game starts
void UOCGTerrainGenerateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UOCGTerrainGenerateComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                 FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UOCGTerrainGenerateComponent::GenerateTerrainInEditor()
{
#if WITH_EDITOR
	const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
	if (!LevelGenerator || !LevelGenerator->GetMapPreset())
		return;
    
	const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

	if (TargetTerrainVolume)
	{
		TargetTerrainVolume->Destroy();
	}
	
	if (TargetVolumeClass)
	{
		TargetTerrainVolume = Cast<AOCGLandscapeVolume>(GetWorld()->SpawnActor(TargetVolumeClass));
	}
	
	if (TargetTerrainVolume)
	{
		if (const ALandscape* Landscape = LevelGenerator->GetLandscape())
		{
			TargetTerrainVolume->AdjustVolumeToBoundsOfActor(Landscape);
		}

		if (UPCGGraph* PCGGraph = MapPreset->PCGGraph)
		{
			UPCGComponent* PCGComponent = TargetTerrainVolume->GetPCGComponent();
			if (PCGComponent)
			{
				PCGComponent->SetGraph(PCGGraph);
				PCGComponent->Generate(true);
			}
		}
	}
#endif
}

void UOCGTerrainGenerateComponent::GenerateTerrain(UWorld* World)
{
#if WITH_EDITOR
	const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
	if (!LevelGenerator || !LevelGenerator->GetMapPreset())
		return;
    
	const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

	if (TargetTerrainVolume)
	{
		TargetTerrainVolume->Destroy();
	}

	if (TargetVolumeClass)
	{
		TargetTerrainVolume = Cast<AOCGLandscapeVolume>(World->SpawnActor(TargetVolumeClass));
	}

	if (TargetTerrainVolume)
	{
		if (const ALandscape* Landscape = LevelGenerator->GetLandscape())
		{
			TargetTerrainVolume->AdjustVolumeToBoundsOfActor(Landscape);
		}

		if (UPCGGraph* PCGGraph = MapPreset->PCGGraph)
		{
			UPCGComponent* PCGComponent = TargetTerrainVolume->GetPCGComponent();
			if (PCGComponent)
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

