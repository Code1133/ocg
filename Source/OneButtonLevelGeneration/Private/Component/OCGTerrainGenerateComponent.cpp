// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/OCGTerrainGenerateComponent.h"

#include "Landscape.h"
#include "OCGLevelGenerator.h"
#include "OCGTerrainVolume.h"
#include "Data/MapPreset.h"


// Sets default values for this component's properties
UOCGTerrainGenerateComponent::UOCGTerrainGenerateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// 일단 CPP로 작성된 Class로 지정
	TargetVolumeClass = AOCGTerrainVolume::StaticClass();
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

	AActor* SpawnedVolume = GetWorld()->SpawnActor(TargetVolumeClass);

	if (AOCGTerrainVolume* TerrainVolume = Cast<AOCGTerrainVolume>(SpawnedVolume))
	{
		if (const ALandscape* Landscape = LevelGenerator->GetLandscape())
		{
			TerrainVolume->AdjustVolumeToBoundsOfActor(Landscape);
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

	AActor* SpawnedVolume = World->SpawnActor(TargetVolumeClass);

	if (AOCGTerrainVolume* TerrainVolume = Cast<AOCGTerrainVolume>(SpawnedVolume))
	{
		if (const ALandscape* Landscape = LevelGenerator->GetLandscape())
		{
			TerrainVolume->AdjustVolumeToBoundsOfActor(Landscape);
		}
	}
#endif
}

AOCGLevelGenerator* UOCGTerrainGenerateComponent::GetLevelGenerator() const
{
	return Cast<AOCGLevelGenerator>(GetOwner());
}

