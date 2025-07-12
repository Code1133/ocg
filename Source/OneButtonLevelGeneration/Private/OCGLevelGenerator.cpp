// Copyright (c) 2025 Code1133. All rights reserved.

#include "OCGLevelGenerator.h"

#include "Landscape.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyCustomActor.h"
#include "WaterBodyOceanActor.h"
#include "WaterBodyOceanComponent.h"
#include "WaterEditorSettings.h"
#include "WaterSplineComponent.h"
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

void AOCGLevelGenerator::SetMapPreset(class UMapPreset* InMapPreset)
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
	check(WaterBodyComponent);
	
	if (const FWaterBodyDefaults* WaterBodyDefaults = &GetDefault<UWaterEditorSettings>()->WaterBodyOceanDefaults)
	{
		WaterBodyComponent->SetWaterMaterial(WaterBodyDefaults->GetWaterMaterial());
		WaterBodyComponent->SetWaterStaticMeshMaterial(WaterBodyDefaults->GetWaterStaticMeshMaterial());
		WaterBodyComponent->SetHLODMaterial(WaterBodyDefaults->GetWaterHLODMaterial());
		WaterBodyComponent->SetUnderwaterPostProcessMaterial(WaterBodyDefaults->GetUnderwaterPostProcessMaterial());

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
		// 2. WaterZone의 ZoneExtent 프로퍼티를 랜드스케이프의 X, Y 크기로 설정합니다.
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
	Params.bShapeOrPositionChanged = true; // Spline 등 Shape이 바뀌었음을 명시
	Params.bUserTriggered = true;          // (선택) 사용자 직접 트리거
	
	InWaterBody->GetWaterBodyComponent()->UpdateAll(Params); // 또는 UpdateAll()
	InWaterBody->GetWaterBodyComponent()->UpdateWaterBodyRenderData();
}

