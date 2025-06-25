// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/MapPreset.h"

#include "Editor/MapPresetEditorToolkit.h"
#include "Materials/MaterialInstanceConstant.h"


#if WITH_EDITOR
void UMapPreset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!PropertyChangedEvent.MemberProperty) //&&  == GET_MEMBER_NAME_CHECKED(ThisClass, HierarchyData)
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PCGHierarchyData))
	{
		// TODO: update volume object
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, PCGGraph))
	{
		// TODO: update volume object
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, LandscapeMaterial))
	{
		if (EditorToolkit)
		{
			EditorToolkit->CreateOrUpdateMaterialEditorWrapper(Cast<UMaterialInstanceConstant>(LandscapeMaterial));
		}
	}
}
#endif
