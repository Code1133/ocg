// Copyright (c) 2025 Code1133. All rights reserved.

#include "Data/MapPreset.h"

#include "OCGCustomVersion.h"
#include "OCGLog.h"
#include "Data/MapData.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"

FOnMapPresetPropertyChanged UMapPreset::OnPropertyChanged = {};

UMapPreset::UMapPreset()
	: OceanWaterMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Water_Material_Ocean.Water_Material_Ocean")))
	, OceanWaterStaticMeshMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/LODs/Water_Material_Ocean_LOD.Water_Material_Ocean_LOD")))
	, WaterHLODMaterial(FSoftObjectPath(TEXT("/Water/Materials/HLOD/HLODWater.HLODWater")))
	, UnderwaterPostProcessMaterial(FSoftObjectPath(TEXT("/Water/Materials/PostProcessing/M_UnderWater_PostProcess_Volume.M_UnderWater_PostProcess_Volume")))
{
}

void UMapPreset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// 저장되는 에셋에 현재 버전 스탬프를 남깁니다. (MapPreset 마이그레이션용)
	Ar.UsingCustomVersion(FOCGCustomVersion::GUID);
}

void UMapPreset::PostLoad()
{
	Super::PostLoad();

	// 버전별 마이그레이션 예시 (River 플랫 필드를 FOCGRiverSettings 묶음으로 옮기는 경우):
	//   1) OCGCustomVersion.h enum의 VersionPlusOne 위에 새 버전을 추가합니다. (LatestVersion은 자동 갱신됩니다)
	//        NestedRiverSettings,
	//   2) MapPreset.h에서 기존 필드는 지우지 않고 deprecated로 남기고, 새로운 필드를 추가합니다.
	//        UPROPERTY() int32 RiverSeed_DEPRECATED = 0;
	//        UPROPERTY(EditAnywhere) FOCGRiverSettings RiverSettings;
	//   3) 저장 당시 버전으로 분기하여 값을 복사합니다. (스탬프 없는 에셋은 BeforeCustomVersionWasAdded로 잡힙니다)
	//        const int32 Ver = GetLinkerCustomVersion(FOCGCustomVersion::GUID);
	//        if (Ver < FOCGCustomVersion::NestedRiverSettings) { RiverSettings.Seed = RiverSeed_DEPRECATED; }

	const int32 Ver = GetLinkerCustomVersion(FOCGCustomVersion::GUID);
	if (Ver < FOCGCustomVersion::NestedSettings)
	{
		ErosionSettings = {
			.bErosion = bErosion_DEPRECATED,
			.NumErosionIterations = NumErosionIterations_DEPRECATED,
			.ErosionRadius = ErosionRadius_DEPRECATED,
			.DropletInertia = DropletInertia_DEPRECATED,
			.SedimentCapacityFactor = SedimentCapacityFactor_DEPRECATED,
			.MinSedimentCapacity = MinSedimentCapacity_DEPRECATED,
			.ErodeSpeed = ErodeSpeed_DEPRECATED,
			.DepositSpeed = DepositSpeed_DEPRECATED,
			.EvaporateSpeed = EvaporateSpeed_DEPRECATED,
			.Gravity = Gravity_DEPRECATED,
			.MaxDropletLifetime = MaxDropletLifetime_DEPRECATED,
			.InitialWaterVolume = InitialWaterVolume_DEPRECATED,
			.InitialSpeed = InitialSpeed_DEPRECATED,
		};
		TemperatureSettings = {
			.MinTemp = MinTemp_DEPRECATED,
			.MaxTemp = MaxTemp_DEPRECATED,
		};
		HumiditySettings = {
			.MoistureFalloffRate = MoistureFalloffRate_DEPRECATED,
			.TemperatureInfluenceOnHumidity = TemperatureInfluenceOnHumidity_DEPRECATED,
		};
		IslandSettings = {
			.bIsland = bIsland_DEPRECATED,
			.IslandFalloffExponent = IslandFalloffExponent_DEPRECATED,
			.IslandShapeNoiseScale = IslandShapeNoiseScale_DEPRECATED,
			.IslandShapeNoiseStrength = IslandShapeNoiseStrength_DEPRECATED,
		};
		BiomeTerrainSettings = {
			.bModifyTerrainByBiome = bModifyTerrainByBiome_DEPRECATED,
			.PlainSmoothFactor = PlainSmoothFactor_DEPRECATED,
			.BiomeNoiseScale = BiomeNoiseScale_DEPRECATED,
			.BiomeNoiseAmplitude = BiomeNoiseAmplitude_DEPRECATED,
			.BiomeHeightBlendRadius = BiomeHeightBlendRadius_DEPRECATED,
		};
		HeightSettings = {
			.MinHeight = MinHeight_DEPRECATED,
			.MaxHeight = MaxHeight_DEPRECATED,
			.SeaLevel = SeaLevel_DEPRECATED,
		};
		BasicNoiseSettings = {
			.ContinentNoiseScale = ContinentNoiseScale_DEPRECATED,
			.TerrainNoiseScale = TerrainNoiseScale_DEPRECATED,
			.TemperatureNoiseScale = TemperatureNoiseScale_DEPRECATED,
		};
		AdvancedNoiseSettings = {
			.StandardNoiseOffset = StandardNoiseOffset_DEPRECATED,
			.RedistributionFactor = RedistributionFactor_DEPRECATED,
			.Octaves = Octaves_DEPRECATED,
			.Lacunarity = Lacunarity_DEPRECATED,
			.Persistence = Persistence_DEPRECATED,
		};
		RiverSettings.bGenerateRiver           = bGenerateRiver_DEPRECATED;
		RiverSettings.RiverSeed                = RiverSeed_DEPRECATED;
		RiverSettings.RiverCount               = RiverCount_DEPRECATED;
		RiverSettings.RiverSourceElevationRatio = RiverSourceElevationRatio_DEPRECATED;
		RiverSettings.RiverSplineSimplifyEpsilon = RiverSplineSimplifyEpsilon_DEPRECATED;
		RiverSettings.RiverWidthBaseValue      = RiverWidthBaseValue_DEPRECATED;
		RiverSettings.RiverDepthBaseValue      = RiverDepthBaseValue_DEPRECATED;
		RiverSettings.RiverVelocityBaseValue   = RiverVelocityBaseValue_DEPRECATED;
		RiverSettings.RiverWidthMin            = RiverWidthMin_DEPRECATED;
		RiverSettings.RiverDepthMin            = RiverDepthMin_DEPRECATED;
		RiverSettings.RiverVelocityMin         = RiverVelocityMin_DEPRECATED;
		RiverSettings.RiverWidthCurve          = RiverWidthCurve_DEPRECATED;
		RiverSettings.RiverDepthCurve          = RiverDepthCurve_DEPRECATED;
		RiverSettings.RiverVelocityCurve       = RiverVelocityCurve_DEPRECATED;
		if (RiverWaterMaterial_DEPRECATED.IsValid())
			RiverSettings.RiverWaterMaterial = RiverWaterMaterial_DEPRECATED;
		if (RiverWaterStaticMeshMaterial_DEPRECATED.IsValid())
			RiverSettings.RiverWaterStaticMeshMaterial = RiverWaterStaticMeshMaterial_DEPRECATED;
		if (RiverToLakeTransitionMaterial_DEPRECATED.IsValid())
			RiverSettings.RiverToLakeTransitionMaterial = RiverToLakeTransitionMaterial_DEPRECATED;
		if (RiverToOceanTransitionMaterial_DEPRECATED.IsValid())
			RiverSettings.RiverToOceanTransitionMaterial = RiverToOceanTransitionMaterial_DEPRECATED;
	}

	UpdateInternalMeshFilterNames();
	UpdateInternalLandscapeFilterNames();
}

void UMapPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty)
	{
		return;
	}

	// 월드 액터 업데이트는 OCGEditorSubsystem 구독자에게 위임합니다.
	// DataAsset은 어느 월드에 속하는지 알 수 없으므로 직접 액터를 조작하지 않습니다.
	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
	OnPropertyChanged.Broadcast(this, PropertyName);

	if (
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, LandscapeMaterial)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, bContainWater)
	)
	{
		UpdateInternalLandscapeFilterNames();
	}

	// Update HierarchiesData
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, HierarchiesData))
	{
		CalculateOptimalLooseness();
		UpdateInternalMeshFilterNames();
		UpdateInternalLandscapeFilterNames();
	}

	// Update Landscape Settings
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Landscape_QuadsPerSection) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Landscape_ComponentCount) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Landscape_SectionsPerComponent) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, MapResolution))
	{
		// Deactivate script execution guard to prevent infinite loop
		FEditorScriptExecutionGuard ScriptGuard;

		// Landscape resolution formula
		// ComponentSize = QuadsPerSection * SectionsPerComponent
		// TotalResolution = ComponentSize * ComponentCount + 1
		const int32 ComponentSize = static_cast<float>(Landscape_QuadsPerSection) * Landscape_SectionsPerComponent;
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, MapResolution))
		{
			// MapResolution이 변경되면 ComponentCount 재계산
			if (ComponentSize > 0)
			{
				FIntPoint NewComponentCount;
				NewComponentCount.X = (MapResolution.X - 1) / ComponentSize;
				NewComponentCount.Y = (MapResolution.Y - 1) / ComponentSize;

				if (Landscape_ComponentCount != NewComponentCount)
				{
					Landscape_ComponentCount = NewComponentCount;
				}
			}
		}
		else
		{
			// 나머지 값이 바뀌었을 경우 MapResolution 재계산
			if (ComponentSize > 0)
			{
				FIntPoint NewMapResolution;
				NewMapResolution.X = ComponentSize * Landscape_ComponentCount.X + 1;
				NewMapResolution.Y = ComponentSize * Landscape_ComponentCount.Y + 1;

				if (MapResolution != NewMapResolution)
				{
					MapResolution = NewMapResolution;
				}
			}
		}

		LandscapeScale = LandscapeSize * 1000.0f / MapResolution.X;

		if (DebugGridSpacing > static_cast<int32>(Landscape_QuadsPerSection))
			DebugGridSpacing = static_cast<int32>(Landscape_QuadsPerSection);
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, HeightmapFilePath))
	{
		if (HeightmapFilePath.FilePath.IsEmpty())
			return;
		FIntPoint HeightmapResolution;
		if (OCGMapDataUtils::GetImageResolution(HeightmapResolution, HeightmapFilePath.FilePath))
		{
			MapResolution = HeightmapResolution;
			const int32 ComponentSize = static_cast<float>(Landscape_QuadsPerSection) * Landscape_SectionsPerComponent;
			FIntPoint NewComponentCount;
			NewComponentCount.X = (MapResolution.X - 1) / ComponentSize;
			NewComponentCount.Y = (MapResolution.Y - 1) / ComponentSize;

			if (Landscape_ComponentCount != NewComponentCount)
			{
				Landscape_ComponentCount = NewComponentCount;
			}
		}
		else
		{
			const FText DialogTitle = FText::FromString(TEXT("Error"));
			const FText DialogText = FText::FromString(TEXT("Failed to read Height Map texture."));

			FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
			return;
		}

		LandscapeScale = LandscapeSize * 1000.0f / MapResolution.X;
	}

	if (PropertyName==GET_MEMBER_NAME_CHECKED(ThisClass, LandscapeSize))
	{
		LandscapeScale = LandscapeSize * 1000.0f / MapResolution.X;
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, DebugGridSpacing))
	{
		if (DebugGridSpacing > static_cast<int32>(Landscape_QuadsPerSection))
			DebugGridSpacing = static_cast<int32>(Landscape_QuadsPerSection);
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Biomes))
	{
		UpdateInternalLandscapeFilterNames();

		if (bContainWater)
		{
			if (Biomes.Num() > 7)
			{
				Biomes.SetNum(7);
				UE_LOG(LogOCGModule, Warning, TEXT("Biomes arrays are allowed up to %d. you have deleted excesses"), 7);
			}
		}
		else
		{
			if (Biomes.Num() > 8)
			{
				Biomes.SetNum(8);
				UE_LOG(LogOCGModule, Warning, TEXT("BioModulemes arrays are allowed up to %d. you have deleted excesses"), 8);
			}
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, bSmoothHeight))
	{
		if (!bSmoothHeight)
		{
			bSmoothBySlope = false;
			bSmoothByMediumHeight = false;
		}
	}
}

void UMapPreset::CalculateOptimalLooseness()
{
	for (FLandscapeHierarchyData& Data : HierarchiesData)
	{
		if (Data.bOverrideLooseness)
		{
			continue;
		}

		const float DesiredSpacing = 0.316 / FMath::Sqrt(Data.PointsPerSquareMeter);
		const float OptimalLooseness = FMath::Clamp(
			DesiredSpacing,
			0.0f,
			5.0f
		);

		Data.Looseness = OptimalLooseness;
	}
}

void UMapPreset::UpdateInternalMeshFilterNames()
{
	for (uint32 Idx = 0; FLandscapeHierarchyData& Data : HierarchiesData)
	{
		Data.MeshFilterName_Internal = FName(*FString::Printf(TEXT("%s_%d"), *Data.BiomeName.ToString(), Idx));
		for (FOCGMeshInfo& Mesh : Data.Meshes)
		{
			// Set MeshFilterName_Internal for each Mesh
			Mesh.MeshFilterName_Internal = Data.MeshFilterName_Internal;
		}
		++Idx;
	}
}

void UMapPreset::UpdateInternalLandscapeFilterNames()
{
	// Make Biome Name To Index Map
	TMap<FName, uint32> NameToIndex;
	for (uint32 Idx = 0; const FOCGBiomeSettings& Data : Biomes)
	{
		NameToIndex.Add(Data.BiomeName, Idx);
		++Idx;
	}

	/* TODO: 추후 LandscapeMaterial이 아니라 bIsOverrideMaterial을 bool로 두고,
	 * OverrideMaterial의 EditCondition 설정해서 사용
	 */

	// Get Landscape Layer Names
	TArray<FName> LandscapeLayerNames;
	if (LandscapeMaterial)
	{
		if (const UMaterial* BaseMaterial = LandscapeMaterial->GetMaterial())
		{
			for (const UMaterialExpression* Expression : BaseMaterial->GetExpressions())
			{
				if (const UMaterialExpressionLandscapeLayerBlend* BlendNode = Cast<UMaterialExpressionLandscapeLayerBlend>(Expression))
				{
					BlendNode->GetLandscapeLayerNames(LandscapeLayerNames);
					break;
				}
			}
		}

		for (FLandscapeHierarchyData& Data : HierarchiesData)
		{
			if (const uint32* Index = NameToIndex.Find(Data.BiomeName))
			{
				const uint32 LayerIdx = *Index + 1;
				if (LandscapeLayerNames.IsValidIndex(LayerIdx))
				{
					Data.LayerName_Internal = LandscapeLayerNames[LayerIdx];
					continue;
				}
			}
			Data.LayerName_Internal = NAME_None;
		}
		return;
	}

	// if LandscapeMaterial is nullptr, use default Layer names
	for (FLandscapeHierarchyData& Data : HierarchiesData)
	{
		// Set LayerName to Layer{idx} for each Biome
		if (const uint32* Index = NameToIndex.Find(Data.BiomeName))
		{
			const uint32 LayerIdx = *Index + 1;
			Data.LayerName_Internal = OCGMapDataUtils::MakeLayerName(LayerIdx);
			continue;
		}
		Data.LayerName_Internal = NAME_None;
	}
}
