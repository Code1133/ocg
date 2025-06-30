// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OCGHierarchyDataAsset.generated.h"


struct FLandscapeHierarchyData;


/**
 * 절차적 레벨 생성을 위한 계층 구조 데이터를 담고 있는 데이터 애셋입니다.
 */
UCLASS(BlueprintType)
class ONEBUTTONLEVELGENERATION_API UOCGHierarchyDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	/** Mesh가 속한 LayerName을 업데이트 합니다. */
	void UpdateMeshLayerNames();

public:
	/** 레벨 생성을 위한 계층 데이터의 배열입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	TArray<FLandscapeHierarchyData> HierarchiesData;
};
