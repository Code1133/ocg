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
}
#endif
