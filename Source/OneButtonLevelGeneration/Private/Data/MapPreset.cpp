// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/MapPreset.h"

#include "EngineUtils.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "PCG/OCGLandscapeVolume.h"

#if WITH_EDITOR
void UMapPreset::PostLoad()
{
	Super::PostLoad();

	UpdateInternalMeshFilterNames();
	UpdateInternalLandscapeFilterNames();
}

void UMapPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty || !EditorToolkit.IsValid())
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();

	// Find Volume Actor
	AActor* FoundActor = nullptr;
	for (AActor* Actor : TActorRange<AActor>(OwnerWorld))
	{
		if (Actor->IsA<AOCGLandscapeVolume>())
		{
			FoundActor = Actor;
			break;
		}
	}

	// Update Volume Actor
	if (AOCGLandscapeVolume* VolumeActor = Cast<AOCGLandscapeVolume>(FoundActor))
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
			VolumeActor->GetPCGComponent()->bRegenerateInEditor = bAutoGenerate;
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

void UMapPreset::ForceGenerate() const
{
	AActor* FoundActor = nullptr;
	for (AActor* Actor : TActorRange<AActor>(OwnerWorld))
	{
		if (Actor->IsA<AOCGLandscapeVolume>())
		{
			FoundActor = Actor;
			break;
		}
	}

	if (const AOCGLandscapeVolume* VolumeActor = Cast<AOCGLandscapeVolume>(FoundActor))
	{
		VolumeActor->GetPCGComponent()->Generate(true);
	}
}
