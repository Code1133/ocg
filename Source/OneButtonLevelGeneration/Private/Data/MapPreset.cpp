// Copyright (c) 2025 Code1133. All rights reserved.

#include "Data/MapPreset.h"

#include "OCGCustomVersion.h"
#include "OCGLog.h"
#include "Data/MapData.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"

FOnMapPresetPropertyChanged UMapPreset::OnPropertyChanged = {};

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
		RiverSettings = {
			.bGenerateRiver = bGenerateRiver_DEPRECATED,
			.RiverSeed = RiverSeed_DEPRECATED,
			.RiverCount = RiverCount_DEPRECATED,
			.RiverSourceElevationRatio = RiverSourceElevationRatio_DEPRECATED,
			.RiverSplineSimplifyEpsilon = RiverSplineSimplifyEpsilon_DEPRECATED,
			.RiverWidthBaseValue = RiverWidthBaseValue_DEPRECATED,
			.RiverDepthBaseValue = RiverDepthBaseValue_DEPRECATED,
			.RiverVelocityBaseValue = RiverVelocityBaseValue_DEPRECATED,
			.RiverWidthMin = RiverWidthMin_DEPRECATED,
			.RiverDepthMin = RiverDepthMin_DEPRECATED,
			.RiverVelocityMin = RiverVelocityMin_DEPRECATED,
			.RiverWidthCurve = RiverWidthCurve_DEPRECATED,
			.RiverDepthCurve = RiverDepthCurve_DEPRECATED,
			.RiverVelocityCurve = RiverVelocityCurve_DEPRECATED,
			.RiverWaterMaterial = RiverWaterMaterial_DEPRECATED.IsValid() ? RiverWaterMaterial_DEPRECATED : nullptr,
			.RiverWaterStaticMeshMaterial = RiverWaterStaticMeshMaterial_DEPRECATED.IsValid() ? RiverWaterStaticMeshMaterial_DEPRECATED : nullptr,
			.RiverToLakeTransitionMaterial = RiverToLakeTransitionMaterial_DEPRECATED.IsValid() ? RiverToLakeTransitionMaterial_DEPRECATED : nullptr,
			.RiverToOceanTransitionMaterial = RiverToOceanTransitionMaterial_DEPRECATED.IsValid() ? RiverToOceanTransitionMaterial_DEPRECATED : nullptr,
		};
		SmoothingSettings = {
			.bSmoothHeight = bSmoothHeight_DEPRECATED,
			.GaussianBlurRadius = GaussianBlurRadius_DEPRECATED,
			.bSmoothBySlope = bSmoothBySlope_DEPRECATED,
			.SmoothingIteration = SmoothingIteration_DEPRECATED,
			.MaxSlopeAngle = MaxSlopeAngle_DEPRECATED,
			.SmoothingStrength = SmoothingStrength_DEPRECATED,
			.bSmoothByMediumHeight = bSmoothByMediumHeight_DEPRECATED,
			.MedianSmoothRadius = MedianSmoothRadius_DEPRECATED,
		};
		OceanSettings = {
			.bContainWater = bContainWater_DEPRECATED,
			.OceanWaterMaterial = OceanWaterMaterial_DEPRECATED.IsValid() ? OceanWaterMaterial_DEPRECATED : nullptr,
			.OceanWaterStaticMeshMaterial = OceanWaterStaticMeshMaterial_DEPRECATED.IsValid() ? OceanWaterStaticMeshMaterial_DEPRECATED : nullptr,
			.WaterHLODMaterial = WaterHLODMaterial_DEPRECATED.IsValid() ? WaterHLODMaterial_DEPRECATED : nullptr,
			.UnderwaterPostProcessMaterial = UnderwaterPostProcessMaterial_DEPRECATED.IsValid() ? UnderwaterPostProcessMaterial_DEPRECATED : nullptr,
		};
		LandscapeSettings = {
			.WorldPartitionGridSize = WorldPartitionGridSize_DEPRECATED,
			.WorldPartitionRegionSize = WorldPartitionRegionSize_DEPRECATED,
			.LandscapeSize = LandscapeSize_DEPRECATED,
			.LandscapeScale = LandscapeScale_DEPRECATED,
			.ApplyScaleToNoise = ApplyScaleToNoise_DEPRECATED,
			.DebugGridSpacing = DebugGridSpacing_DEPRECATED,
			.BiomeBlendRadius = BiomeBlendRadius_DEPRECATED,
			.WaterBlendRadius = WaterBlendRadius_DEPRECATED,
			.Landscape_QuadsPerSection = Landscape_QuadsPerSection_DEPRECATED,
			.Landscape_SectionsPerComponent = Landscape_SectionsPerComponent_DEPRECATED,
			.Landscape_ComponentCount = Landscape_ComponentCount_DEPRECATED,
			.MapResolution = MapResolution_DEPRECATED,
			.LandscapeMaterial = LandscapeMaterial_DEPRECATED,
			.HeightmapFilePath = HeightmapFilePath_DEPRECATED,
		};
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

	// Ocean Settings 변경 -> 내부 레이어 필터 이름 갱신
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, OceanSettings))
	{
		UpdateInternalLandscapeFilterNames();
	}

	// HierarchiesData 변경 -> 루즈니스 재계산 + 필터 이름 갱신
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, HierarchiesData))
	{
		CalculateOptimalLooseness();
		UpdateInternalMeshFilterNames();
		UpdateInternalLandscapeFilterNames();
	}

	// Landscape Settings 변경
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, LandscapeSettings))
	{
		const FName SubPropertyName = PropertyChangedEvent.GetPropertyName();

		// LandscapeMaterial -> 내부 레이어 필터 이름 갱신
		if (SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, LandscapeMaterial))
		{
			UpdateInternalLandscapeFilterNames();
		}

		// 해상도 관련 파라미터 변경 -> MapResolution / ComponentCount 상호 재계산
		if (
			SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, Landscape_QuadsPerSection)
			|| SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, Landscape_ComponentCount)
			|| SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, Landscape_SectionsPerComponent)
			|| SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, MapResolution)
		)
		{
			// 무한 루프 방지용 가드
			FEditorScriptExecutionGuard ScriptGuard;

			// Landscape resolution formula
			// ComponentSize = QuadsPerSection * SectionsPerComponent
			// TotalResolution = ComponentSize * ComponentCount + 1
			const int32 ComponentSize = static_cast<float>(LandscapeSettings.Landscape_QuadsPerSection) * LandscapeSettings.Landscape_SectionsPerComponent;
			if (SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, MapResolution))
			{
				// MapResolution이 변경되면 ComponentCount 재계산
				if (ComponentSize > 0)
				{
					FIntPoint NewComponentCount;
					NewComponentCount.X = (LandscapeSettings.MapResolution.X - 1) / ComponentSize;
					NewComponentCount.Y = (LandscapeSettings.MapResolution.Y - 1) / ComponentSize;

					if (LandscapeSettings.Landscape_ComponentCount != NewComponentCount)
					{
						LandscapeSettings.Landscape_ComponentCount = NewComponentCount;
					}
				}
			}
			else
			{
				// 나머지 값이 바뀌었을 경우 MapResolution 재계산
				if (ComponentSize > 0)
				{
					FIntPoint NewMapResolution;
					NewMapResolution.X = ComponentSize * LandscapeSettings.Landscape_ComponentCount.X + 1;
					NewMapResolution.Y = ComponentSize * LandscapeSettings.Landscape_ComponentCount.Y + 1;

					if (LandscapeSettings.MapResolution != NewMapResolution)
					{
						LandscapeSettings.MapResolution = NewMapResolution;
					}
				}
			}

			LandscapeSettings.LandscapeScale = LandscapeSettings.LandscapeSize * 1000.0f / LandscapeSettings.MapResolution.X;

			if (LandscapeSettings.DebugGridSpacing > static_cast<int32>(LandscapeSettings.Landscape_QuadsPerSection))
			{
				LandscapeSettings.DebugGridSpacing = static_cast<int32>(LandscapeSettings.Landscape_QuadsPerSection);
			}
		}

		// HeightmapFilePath 변경 -> 파일에서 해상도 검출
		if (SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, HeightmapFilePath))
		{
			if (LandscapeSettings.HeightmapFilePath.FilePath.IsEmpty())
			{
				return;
			}

			FIntPoint HeightmapResolution;
			if (OCGMapDataUtils::GetImageResolution(HeightmapResolution, LandscapeSettings.HeightmapFilePath.FilePath))
			{
				LandscapeSettings.MapResolution = HeightmapResolution;
				const int32 ComponentSize = static_cast<float>(LandscapeSettings.Landscape_QuadsPerSection) * LandscapeSettings.Landscape_SectionsPerComponent;
				FIntPoint NewComponentCount;
				NewComponentCount.X = (LandscapeSettings.MapResolution.X - 1) / ComponentSize;
				NewComponentCount.Y = (LandscapeSettings.MapResolution.Y - 1) / ComponentSize;

				if (LandscapeSettings.Landscape_ComponentCount != NewComponentCount)
				{
					LandscapeSettings.Landscape_ComponentCount = NewComponentCount;
				}
			}
			else
			{
				const FText DialogTitle = FText::FromString(TEXT("Error"));
				const FText DialogText = FText::FromString(TEXT("Failed to read Height Map texture."));
				FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
				return;
			}
			LandscapeSettings.LandscapeScale = LandscapeSettings.LandscapeSize * 1000.0f / LandscapeSettings.MapResolution.X;
		}

		// LandscapeSize 변경 -> LandscapeScale 재계산
		if (SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, LandscapeSize))
		{
			LandscapeSettings.LandscapeScale = LandscapeSettings.LandscapeSize * 1000.0f / LandscapeSettings.MapResolution.X;
		}

		// DebugGridSpacing 클램프
		if (SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGLandscapeSettings, DebugGridSpacing))
		{
			if (LandscapeSettings.DebugGridSpacing > static_cast<int32>(LandscapeSettings.Landscape_QuadsPerSection))
				LandscapeSettings.DebugGridSpacing = static_cast<int32>(LandscapeSettings.Landscape_QuadsPerSection);
		}
	}

	// Biomes 변경 -> 바이옴 수 제한 적용
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Biomes))
	{
		UpdateInternalLandscapeFilterNames();

		if (OceanSettings.bContainWater)
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
				UE_LOG(LogOCGModule, Warning, TEXT("Biomes arrays are allowed up to %d. you have deleted excesses"), 8);
			}
		}
	}

	// Smoothing Settings 변경
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, SmoothingSettings))
	{
		const FName SubPropertyName = PropertyChangedEvent.GetPropertyName();
		if (SubPropertyName == GET_MEMBER_NAME_CHECKED(FOCGSmoothingSettings, bSmoothHeight))
		{
			if (!SmoothingSettings.bSmoothHeight)
			{
				SmoothingSettings.bSmoothBySlope = false;
				SmoothingSettings.bSmoothByMediumHeight = false;
			}
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
	if (LandscapeSettings.LandscapeMaterial)
	{
		if (const UMaterial* BaseMaterial = LandscapeSettings.LandscapeMaterial->GetMaterial())
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
