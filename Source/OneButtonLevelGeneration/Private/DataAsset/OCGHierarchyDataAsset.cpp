// Copyright (c) 2025 Code1133. All rights reserved.

#include "DataAsset/OCGHierarchyDataAsset.h"


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
		&& PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(ThisClass, HierarchyData)
	)
	{
		UpdateMeshLayerNames();
	}
}
#endif

void UOCGHierarchyDataAsset::UpdateMeshLayerNames()
{
	for (FLandscapeHierarchyData& Data : HierarchyData)
	{
		for (FMeshAndWeight& Mesh : Data.Meshes)
		{
			// 부모 레이어 이름으로 자동 설정
			Mesh.LayerName = Data.LayerName;
		}
	}
}
