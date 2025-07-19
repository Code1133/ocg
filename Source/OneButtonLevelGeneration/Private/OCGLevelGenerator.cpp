// Copyright (c) 2025 Code1133. All rights reserved.

#include "OCGLevelGenerator.h"

#include <OCGLog.h>

#include "Landscape.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyOceanActor.h"
#include "WaterBodyOceanComponent.h"
#include "WaterEditorSettings.h"
#include "WaterSplineComponent.h"
#include "Component/OCGMapGenerateComponent.h"
#include "Component/OCGLandscapeGenerateComponent.h"
#include "Component/OCGRiverGeneratorComponent.h"
#include "Component/OCGTerrainGenerateComponent.h"
#include "Data/MapPreset.h"

AOCGLevelGenerator::AOCGLevelGenerator()
{
	MapGenerateComponent = CreateDefaultSubobject<UOCGMapGenerateComponent>(TEXT("MapGenerateComponent"));
	LandscapeGenerateComponent = CreateDefaultSubobject<UOCGLandscapeGenerateComponent>(TEXT("LandscapeGenerateComponent"));
	TerrainGenerateComponent = CreateDefaultSubobject<UOCGTerrainGenerateComponent>(TEXT("TerrainGenerateComponent"));
	RiverGenerateComponent = CreateDefaultSubobject<UOCGRiverGenerateComponent>(TEXT("RiverGenerateComponent"));
	SetIsSpatiallyLoaded(false);
}

void AOCGLevelGenerator::Generate()
{
	if (MapGenerateComponent)
	{
		MapGenerateComponent->GenerateMaps();
	}
	
	if (LandscapeGenerateComponent)
	{
		LandscapeGenerateComponent->SetLandscapeZValues(MapGenerateComponent->GetZScale(), MapGenerateComponent->GetZOffset());
		LandscapeGenerateComponent->GenerateLandscape(GetWorld());
	}

	if (TerrainGenerateComponent)
	{
		TerrainGenerateComponent->GenerateTerrain(GetWorld());
	}

	AddWaterPlane(GetWorld());
}

void AOCGLevelGenerator::OnClickGenerate(UWorld* InWorld)
{
	if (!MapPreset)
	{
		UE_LOG(LogOCGModule, Error, TEXT("MapPreset is not set! Please set a valid MapPreset before generating."));
		return;
	}
	
	if (!MapPreset || MapPreset->Biomes.IsEmpty())
	{
		// Error message
		const FText DialogTitle = FText::FromString(TEXT("Error"));
		const FText DialogText = FText::FromString(TEXT("At Least one biome must be defined in the preset before generating the level."));

		FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);

		return;
	}
	
	for (const auto& Biome : MapPreset->Biomes)
	{
		if (Biome.BiomeName == NAME_None)
		{
			const FText DialogTitle = FText::FromString(TEXT("Error"));
			const FText DialogText = FText::FromString(TEXT("Invalid Biome Name. Please set a valid name for each biome."));

			FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
			return;
		}
	}

	if (MapGenerateComponent)
	{
		MapGenerateComponent->GenerateMaps();
	}

	if (LandscapeGenerateComponent)
	{
		LandscapeGenerateComponent->SetLandscapeZValues(MapGenerateComponent->GetZScale(), MapGenerateComponent->GetZOffset());
		LandscapeGenerateComponent->GenerateLandscape(InWorld);
	}

	if (TerrainGenerateComponent)
	{
		TerrainGenerateComponent->GenerateTerrain(InWorld);
	}

	if (RiverGenerateComponent && MapGenerateComponent && LandscapeGenerateComponent && MapPreset)
	{
		RiverGenerateComponent->SetMapData(
			MapPreset->HeightMapData,
			MapPreset, 
			MapPreset->CurMinHeight,
			MapPreset->CurMaxHeight
		);

		RiverGenerateComponent->GenerateRiver(InWorld, LandscapeGenerateComponent->GetLandscape());
	}
	
	AddWaterPlane(InWorld);
}

const TArray<uint16>& AOCGLevelGenerator::GetHeightMapData() const
{
	return MapPreset->HeightMapData;
}

const TArray<uint16>& AOCGLevelGenerator::GetTemperatureMapData() const
{
	return MapPreset->TemperatureMapData;
}

const TArray<uint16>& AOCGLevelGenerator::GetHumidityMapData() const
{
	return MapPreset->HumidityMapData;
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

FVector AOCGLevelGenerator::GetVolumeExtent() const
{
	if (LandscapeGenerateComponent)
	{
		return LandscapeGenerateComponent->GetVolumeExtent();
	}
	return FVector();
}

FVector AOCGLevelGenerator::GetVolumeOrigin() const
{
	if (LandscapeGenerateComponent)
	{
		return LandscapeGenerateComponent->GetVolumeOrigin();
	}
	return FVector();
}

void AOCGLevelGenerator::SetMapPreset(UMapPreset* InMapPreset)
{
	MapPreset = InMapPreset;
	MapPreset->LandscapeGenerator = this;
}

void AOCGLevelGenerator::AddWaterPlane(UWorld* InWorld)
{
	if (SeaLevelWaterBody)
	{
		SeaLevelWaterBody->Destroy();
		SeaLevelWaterBody = nullptr;
	}

	if (!InWorld || !MapPreset || !MapPreset->bContainWater)
	{
		return;
	}

	SeaLevelWaterBody = InWorld->SpawnActor<AWaterBodyOcean>(AWaterBodyOcean::StaticClass());
	SeaLevelWaterBody->SetIsSpatiallyLoaded(false);
	SetDefaultWaterProperties(SeaLevelWaterBody);
	
	// Linear Interpolation for sea height
	float SeaHeight = MapPreset->MinHeight + 
		(MapPreset->MaxHeight - MapPreset->MinHeight) * MapPreset->SeaLevel - 5;
	// FTransform SeaLevelWaterbodyTransform = FTransform::Identity;

	SeaLevelWaterBody->SetActorLocation(FVector(0.0f, 0.0f, SeaHeight));
	
	ALandscape* Landscape = LandscapeGenerateComponent->GetLandscape();
	if (Landscape)
	{
		FVector LandscapeOrigin = GetVolumeOrigin();
		FVector LandscapeExtent = GetVolumeExtent();

		SeaLevelWaterBody->SetActorLocation(FVector(LandscapeOrigin.X, LandscapeOrigin.Y, SeaHeight));
		
		const float ScaleX = (LandscapeExtent.X * 2.0f) / 100.0f;
		const float ScaleY = (LandscapeExtent.Y * 2.0f) / 100.0f;
		
		const FVector RequiredScale(ScaleX, ScaleY, 1.0f);
		
		SeaLevelWaterBody->SetActorScale3D(RequiredScale);
	}
}

void AOCGLevelGenerator::SetDefaultWaterProperties(AWaterBody* InWaterBody)
{
	UWaterBodyComponent* WaterBodyComponent = CastChecked<AWaterBody>(InWaterBody)->GetWaterBodyComponent();
	check(MapPreset && WaterBodyComponent);

	WaterBodyComponent->SetWaterMaterial(MapPreset->OceanWaterMaterial.LoadSynchronous());
	WaterBodyComponent->SetWaterStaticMeshMaterial(MapPreset->OceanWaterStaticMeshMaterial.LoadSynchronous());
	WaterBodyComponent->SetHLODMaterial(MapPreset->WaterHLODMaterial.LoadSynchronous());
	WaterBodyComponent->SetUnderwaterPostProcessMaterial(MapPreset->UnderwaterPostProcessMaterial.LoadSynchronous());
	
	if (const FWaterBodyDefaults* WaterBodyDefaults = &GetDefault<UWaterEditorSettings>()->WaterBodyOceanDefaults)
	{
		UWaterSplineComponent* WaterSpline = WaterBodyComponent->GetWaterSpline();
		WaterSpline->WaterSplineDefaults = WaterBodyDefaults->SplineDefaults;
	}

	// If the water body is spawned into a zone which is using local only tessellation, we must default to enabling static meshes.
	if (AWaterZone* WaterZone = WaterBodyComponent->GetWaterZone())
	{
		if (WaterZone->IsLocalOnlyTessellationEnabled())
		{
			WaterBodyComponent->SetWaterBodyStaticMeshEnabled(true);
		}

		FVector Extent = GetLandscape()->GetLoadedBounds().GetExtent();
		WaterZone->SetZoneExtent(FVector2D(Extent.X * 2, Extent.Y * 2));
	}

	AWaterBodyOcean* WaterBodyOcean = Cast<AWaterBodyOcean>(InWaterBody);
	if (const UWaterWavesBase* DefaultWaterWaves = GetDefault<UWaterEditorSettings>()->WaterBodyOceanDefaults.WaterWaves)
	{
		UWaterWavesBase* WaterWaves = DuplicateObject(DefaultWaterWaves, InWaterBody, MakeUniqueObjectName(InWaterBody, DefaultWaterWaves->GetClass(), TEXT("OceanWaterWaves")));
		WaterBodyOcean->SetWaterWaves(WaterWaves);
	}

	UWaterSplineComponent* WaterSpline = WaterBodyComponent->GetWaterSpline();
	WaterSpline->ResetSpline({FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector, FVector::ZeroVector });

	if (const AWaterZone* OwningWaterZone = WaterBodyComponent->GetWaterZone())
	{
		if (UWaterBodyOceanComponent* OceanComponent = Cast<UWaterBodyOceanComponent>(WaterBodyComponent))
		{
			const double ExistingCollisionHeight = OceanComponent->GetCollisionExtents().Z;
			OceanComponent->bAffectsLandscape = false;
			OceanComponent->SetCollisionExtents(FVector(OwningWaterZone->GetZoneExtent() / 2.0, ExistingCollisionHeight));
			OceanComponent->FillWaterZoneWithOcean();
		}
	}

	InWaterBody->PostEditChange();
	InWaterBody->PostEditMove(true);

	FOnWaterBodyChangedParams Params;
	Params.bShapeOrPositionChanged = true; 
	Params.bUserTriggered = true;          
	
	InWaterBody->GetWaterBodyComponent()->UpdateAll(Params); 
	InWaterBody->GetWaterBodyComponent()->UpdateWaterBodyRenderData();
}

