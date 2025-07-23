// Copyright (c) 2025 Code1133. All rights reserved.

#include "Data/MapPreset.h"

#include "OCGLevelGenerator.h"
#include "OCGLog.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "ToolContextInterfaces.h"
#include "Component/OCGMapGenerateComponent.h"
#include "Component/OCGRiverGeneratorComponent.h"
#include "Data/MapData.h"
#include "Editor/SOCGWidget.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Utils/OCGUtils.h"

#if WITH_EDITOR
UMapPreset::UMapPreset()
	: OceanWaterMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Water_Material_Ocean.Water_Material_Ocean")))
	, OceanWaterStaticMeshMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/LODs/Water_Material_Ocean_LOD.Water_Material_Ocean_LOD")))
	, WaterHLODMaterial(FSoftObjectPath(TEXT("/Water/Materials/HLOD/HLODWater.HLODWater")))
	, UnderwaterPostProcessMaterial(FSoftObjectPath(TEXT("/Water/Materials/PostProcessing/M_UnderWater_PostProcess_Volume.M_UnderWater_PostProcess_Volume")))
	, RiverWaterMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Water_Material_River.Water_Material_River")))
	, RiverWaterStaticMeshMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/LODs/Water_Material_River_LOD.Water_Material_River_LOD")))
	, RiverToLakeTransitionMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Transitions/Water_Material_River_To_Lake_Transition.Water_Material_River_To_Lake_Transition")))
	, RiverToOceanTransitionMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Transitions/Water_Material_River_To_Ocean_Transition.Water_Material_River_To_Ocean_Transition")))
{
}

void UMapPreset::PostLoad()
{
	Super::PostLoad();

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

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();

	// Find Volume Actor
	TArray<AOCGLandscapeVolume*> Actors;
	if (UWorld* World = GetWorld())
	{
		Actors = FOCGUtils::GetAllActorsOfClass<AOCGLandscapeVolume>(World);
	}

	// Update Volume Actor
	for (AOCGLandscapeVolume* VolumeActor : Actors)
	{
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PCGGraph))
		{
			VolumeActor->GetPCGComponent()->SetGraph(PCGGraph);
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, bAutoGenerate))
		{
			VolumeActor->SetEditorAutoGenerate(bAutoGenerate);
		}
	}

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
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, HeightmapFilePath))
	{
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
#endif

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
			Data.LayerName_Internal = FName(*FString::Printf(TEXT("Layer%d"), LayerIdx));
			continue;
		}
		Data.LayerName_Internal = NAME_None;
	}
}

UWorld* UMapPreset::GetWorld() const
{
#if WITH_EDITOR
	if (LandscapeGenerator.IsValid())
	{
		return LandscapeGenerator->GetWorld();
	}
	return GEditor->GetEditorWorldContext().World();
#else
	return nullptr;
#endif
}

void UMapPreset::PreviewMaps()
{
	if (Biomes.IsEmpty())
	{
		// Error message
		const FText DialogTitle = FText::FromString(TEXT("Error"));
		const FText DialogText = FText::FromString(TEXT("At Least one biome must be defined in the preset before generating the level."));

		FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);

		return;
	}
	
	for (const auto& Biome : Biomes)
	{
		if (Biome.BiomeName == NAME_None)
		{
			const FText DialogTitle = FText::FromString(TEXT("Error"));
			const FText DialogText = FText::FromString(TEXT("Invalid Biome Name. Please set a valid name for each biome."));

			FMessageDialog::Open(EAppMsgType::Ok, DialogText, DialogTitle);
			return;
		}
	}
	bool bOriginalExportSetting = bExportMapTextures;
	if (!bOriginalExportSetting)
		bExportMapTextures = true;
	if (LandscapeGenerator.IsValid())
	{
		LandscapeGenerator->GetMapGenerateComponent()->GenerateMaps();
	}
	bExportMapTextures = bOriginalExportSetting;
}
