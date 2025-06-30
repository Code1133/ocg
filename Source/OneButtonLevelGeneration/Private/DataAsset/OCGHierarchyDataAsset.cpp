// Copyright (c) 2025 Code1133. All rights reserved.

#include "DataAsset/OCGHierarchyDataAsset.h"

#include "Structure/OCGHierarchyDataStructure.h"


struct FOCGMeshInfo;

void UOCGHierarchyDataAsset::PostLoad()
{
	Super::PostLoad();

	UpdateMeshLayerNames();
}

#if WITH_EDITOR
void UOCGHierarchyDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (
		PropertyChangedEvent.MemberProperty
		&& PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, HierarchiesData)
	)
	{
		UpdateMeshLayerNames();
	}
}
#endif

void UOCGHierarchyDataAsset::UpdateMeshLayerNames()
{
	for (uint32 Idx = 0; FLandscapeHierarchyData& Data : HierarchiesData)
	{
		Data.MeshFilterName_Internal = FName(*FString::Printf(TEXT("%s_%d"), *Data.LayerName.ToString(), Idx));
		for (FOCGMeshInfo& Mesh : Data.Meshes)
		{
			// 부모 레이어 이름으로 자동 설정
			Mesh.MeshFilterName_Internal = Data.MeshFilterName_Internal;
		}
		++Idx;
	}
}
