// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/MapPreset.h"

#include "EngineUtils.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Editor/MapPresetEditorToolkit.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PCG/OCGLandscapeVolume.h"


#if WITH_EDITOR
void UMapPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty || !EditorToolkit)
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
		if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PCGHierarchyData))
		{
			VolumeActor->DataAsset = PCGHierarchyData;
		}
		else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PCGGraph))
		{
			VolumeActor->GetPCGComponent()->SetGraph(PCGGraph);
		}
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, LandscapeMaterial))
	{
		EditorToolkit->CreateOrUpdateMaterialEditorWrapper(Cast<UMaterialInstanceConstant>(LandscapeMaterial));
	}

	//Update Landscape Settings
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Landscape_QuadsPerSection) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Landscape_ComponentCount) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Landscape_SectionsPerComponent) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, MapResolution))
	{
		// 무한 루프를 방지하기 위해 변경 알림을 잠시 비활성화합니다.
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
}
#endif
