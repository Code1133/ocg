// Copyright (c) 2025 Code1133. All rights reserved.

#include "Data/MapPreset.h"

#include "OCGLevelGenerator.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Component/OCGRiverGeneratorComponent.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Utils/OCGUtils.h"

#if WITH_EDITOR
UMapPreset::UMapPreset()
	: OceanWaterMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Water_Material_Ocean.Water_Material_Ocean")))
	, OceanWaterStaticMeshMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/LODs/Water_Material_Ocean_LOD.Water_Material_Ocean_LOD")))
	, RiverWaterMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Water_Material_River.Water_Material_River")))
	, RiverWaterStaticMeshMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/LODs/Water_Material_River_LOD.Water_Material_River_LOD")))
	, WaterHLODMaterial(FSoftObjectPath(TEXT("/Water/Materials/HLOD/HLODWater.HLODWater")))
	, UnderwaterPostProcessMaterial(FSoftObjectPath(TEXT("/Water/Materials/PostProcessing/M_UnderWater_PostProcess_Volume.M_UnderWater_PostProcess_Volume")))
	, RiverToOceanTransitionMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Transitions/Water_Material_River_To_Ocean_Transition.Water_Material_River_To_Ocean_Transition")))
	, RiverToLakeTransitionMaterial(FSoftObjectPath(TEXT("/Water/Materials/WaterSurface/Transitions/Water_Material_River_To_Lake_Transition.Water_Material_River_To_Lake_Transition")))
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
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, bShowDebugPoints))
		{
			VolumeActor->bShowDebugPoints = bShowDebugPoints;
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
		int32 ComponentSize;
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, MapResolution))
		{
			ComponentSize = static_cast<float>(Landscape_QuadsPerSection) * Landscape_SectionsPerComponent;
			FIntPoint ComponentCount;
			ComponentCount.X = (MapResolution.X - 1) / ComponentSize;
			ComponentCount.Y = (MapResolution.Y - 1) / ComponentSize;
			Landscape_ComponentCount = ComponentCount;
		}
		else
		{
			ComponentSize = static_cast<float>(Landscape_QuadsPerSection) * Landscape_SectionsPerComponent;
			FIntPoint NewMapResolution;
			NewMapResolution.X = ComponentSize * Landscape_ComponentCount.X + 1;
			NewMapResolution.Y = ComponentSize * Landscape_ComponentCount.Y + 1;
			MapResolution = NewMapResolution;
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
				UE_LOG(LogTemp, Warning, TEXT("Biomes arrays are allowed up to %d. you have deleted excesses"), 7);
			}
		}
		else
		{
			if (Biomes.Num() > 8)
			{
				Biomes.SetNum(8);
				UE_LOG(LogTemp, Warning, TEXT("Biomes arrays are allowed up to %d. you have deleted excesses"), 8);
			}
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
			Data.LayerName_Internal = FName(*FString::Printf(TEXT("Layer%d"), *Index));
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

void UMapPreset::ForceGenerate() const
{
	if (UWorld* World = GetWorld())
	{
		const TArray<AOCGLandscapeVolume*> Actors =
			FOCGUtils::GetAllActorsOfClass<AOCGLandscapeVolume>(World);

		for (const AOCGLandscapeVolume* VolumeActor : Actors)
		{
			VolumeActor->GetPCGComponent()->Generate(true);
		}
	}
}

void UMapPreset::RegenerateRiver()
{
	if (UWorld* World = GetWorld())
	{
		TArray<AOCGLevelGenerator*> Actors =
			FOCGUtils::GetAllActorsOfClass<AOCGLevelGenerator>(World);

		for (AOCGLevelGenerator* LevelGenerator : Actors)
		{
			LevelGenerator->GetRiverGenerateComponent()->GenerateRiver(World, LevelGenerator->GetLandscape());
		}
	}
}
