// Copyright (c) 2025 Code1133. All rights reserved.

#include "OCGLevelGenerator.h"

#include "Landscape.h"
#include "Component/OCGMapGenerateComponent.h"
#include "Component/OCGLandscapeGenerateComponent.h"
#include "Component/OCGRiverGeneratorComponent.h"
#include "Component/OCGTerrainGenerateComponent.h"
#include "Data/MapPreset.h"
#include "Engine/StaticMeshActor.h"

AOCGLevelGenerator::AOCGLevelGenerator()
{
	MapGenerateComponent = CreateDefaultSubobject<UOCGMapGenerateComponent>(TEXT("MapGenerateComponent"));
	LandscapeGenerateComponent = CreateDefaultSubobject<UOCGLandscapeGenerateComponent>(TEXT("LandscapeGenerateComponent"));
	TerrainGenerateComponent = CreateDefaultSubobject<UOCGTerrainGenerateComponent>(TEXT("TerrainGenerateComponent"));
	RiverGenerateComponent = CreateDefaultSubobject<UOCGRiverGeneratorComponent>(TEXT("RiverGenerateComponent"));
}

void AOCGLevelGenerator::Generate()
{
	if (MapGenerateComponent)
	{
		MapGenerateComponent->GenerateMaps();
	}

	if (LandscapeGenerateComponent)
	{
		LandscapeGenerateComponent->GenerateLandscape(GetWorld());
	}

	if (TerrainGenerateComponent)
	{
		
	}
}

void AOCGLevelGenerator::OnClickGenerate(UWorld* InWorld)
{
	if (MapGenerateComponent)
	{
		MapGenerateComponent->GenerateMaps();
	}

	if (LandscapeGenerateComponent)
	{
		LandscapeGenerateComponent->GenerateLandscape(InWorld);
	}

	if (TerrainGenerateComponent)
	{
		TerrainGenerateComponent->GenerateTerrain(InWorld);
	}

	if (RiverGenerateComponent && MapGenerateComponent && LandscapeGenerateComponent && MapPreset)
	{
		float SeaHeight = MapPreset->MinHeight +
			(MapPreset->MaxHeight - MapPreset->MinHeight) * MapPreset->SeaLevel - 1;

		RiverGenerateComponent->SetMapData(
			MapGenerateComponent->GetHeightMapData(),
			MapPreset
		);

		RiverGenerateComponent->GenerateRiver(InWorld, LandscapeGenerateComponent->GetLandscape());
	}

	AddWaterPlane(InWorld);
}

const TArray<uint16>& AOCGLevelGenerator::GetHeightMapData() const
{
	return MapGenerateComponent->GetHeightMapData();
}

const TArray<uint16>& AOCGLevelGenerator::GetTemperatureMapData() const
{
	return MapGenerateComponent->GetTemperatureMapData();
}

const TArray<uint16>& AOCGLevelGenerator::GetHumidityMapData() const
{
	return MapGenerateComponent->GetHumidityMapData();
}

const TMap<FName, TArray<uint8>>& AOCGLevelGenerator::GetWeightLayers() const
{
	return MapGenerateComponent->GetWeightLayers();
}

const ALandscape* AOCGLevelGenerator::GetLandscape() const
{
	return LandscapeGenerateComponent->GetLandscape();
}

ALandscape* AOCGLevelGenerator::GetLandscape()
{
	return LandscapeGenerateComponent->GetLandscape();
}

void AOCGLevelGenerator::SetMapPreset(class UMapPreset* InMapPreset)
{
	MapPreset = InMapPreset;
}

void AOCGLevelGenerator::AddWaterPlane(UWorld* InWorld)
{
	if (PlaneActor)
	{
		PlaneActor->Destroy();
		PlaneActor = nullptr;
	}

	if (!InWorld || !MapPreset || !MapPreset->bContainWater)
	{
		return;
	}

	// GetPlaneStaticMesh
	const FString PlaneMeshPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, *PlaneMeshPath);
	
	if (!PlaneMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Plane mesh at path: %s"), *PlaneMeshPath);
		return;
	}

	const FString WaterMaterialPath = TEXT("/DatasmithContent/Materials/Water/M_Water.M_Water");
	UMaterialInterface* WaterMaterial = LoadObject<UMaterialInterface>(nullptr, *WaterMaterialPath);

	if (!WaterMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Water material at path: %s"), *WaterMaterialPath);
	}

	// Linear Interpolation for sea height
	float SeaHeight = MapPreset->MinHeight + 
		(MapPreset->MaxHeight - MapPreset->MinHeight) * MapPreset->SeaLevel - 1;
	SeaHeight *= MapPreset->LandscapeScale;	
	FTransform PlaneTransform = FTransform::Identity;
	
	PlaneTransform.SetLocation(FVector(0.0f, 0.0f, SeaHeight));
	
	PlaneActor = InWorld->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), PlaneTransform);
	
	ALandscape* Landscape = LandscapeGenerateComponent->GetLandscape();
	if (Landscape)
	{
		FVector Extent = Landscape->GetLoadedBounds().GetExtent();
		
		const float ScaleX = (Extent.X * 2.0f) / 100.0f;
		const float ScaleY = (Extent.Y * 2.0f) / 100.0f;
		
		const FVector RequiredScale(ScaleX, ScaleY, 1.0f);
		
		PlaneActor->SetActorScale3D(RequiredScale);
	}

	UStaticMeshComponent* MeshComponent = PlaneActor->GetStaticMeshComponent();
	if (MeshComponent)
	{
		MeshComponent->SetStaticMesh(PlaneMesh);
		MeshComponent->SetMaterial(0, WaterMaterial);
	}
}

