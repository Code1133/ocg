// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OCGHierarchyDataAsset.generated.h"


/**
 * 메시와 해당 메시의 생성 가중치를 정의하는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FMeshAndWeight
{
	GENERATED_BODY()

	/** 배치할 스태틱 메시입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh;

	/** 메시 생성 가중치입니다. 다른 메시와 비교하여 이 메시가 선택될 확률입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, UIMin = 0))
	int32 Weight = 1;

	/** Mesh의 기본 Collision을 활성화 합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bEnableDefaultCollision = false;

	/** 메시가 속할 레이어의 이름입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LayerName;
};

/**
 * 랜드스케이프의 특정 레이어에 대한 계층적 생성 데이터를 정의하는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FLandscapeHierarchyData
{
	GENERATED_BODY()

	/** 이 계층 데이터가 나타내는 레이어의 이름입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LayerName;

	/** 블렌딩된 레이어의 가중치입니다. 0과 1 사이의 값으로, 값이 낮을수록 이 레이어의 영향력이 커집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float BlendingRatio = 0.5f;

	/** 제곱미터당 생성할 포인트의 수입니다. 포인트 밀도를 결정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PointsPerSquareMeter = 0.1f;

	/** 각 포인트의 영향 범위(반경)입니다. X, Y, Z 축으로 설정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector PointExtent = FVector{100.0f, 100.0f, 100.0f};

	/** 포인트 배치의 불규칙성 또는 '느슨함'의 정도입니다. 값이 높을수록 포인트가 더 넓게 퍼집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Looseness = 1.0f;

	/** 포인트의 가파른 정도를 제어합니다. 0과 1 사이의 값으로, 1에 가까울수록 경사가 급해집니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float PointSteepness = 0.5f;

	/** 월드 위치 오프셋 기능이 비활성화되는 거리입니다. 0이면 항상 활성화 됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 WorldPositionOffsetDisableDistance = 0;

	/** Mesh를 Cull할 시작 거리입니다. 0이면 항상 활성화 됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StartCullDistance = 0;

	/** Mesh를 Cull할 끝 거리입니다. 0이면 항상 활성화 됩니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 EndCullDistance = 0;

	/** 포인트 생성을 위한 랜덤 시드입니다. 동일한 시드는 동일한 결과를 생성합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Seed;

	/** 디버깅 시각화를 위해 이 레이어를 나타내는 색상입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor DebugColor = FLinearColor::White;

	/** 이 레이어에 배치될 메시와 가중치의 배열입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMeshAndWeight> Meshes;

	FLandscapeHierarchyData()
		: Seed(FMath::Rand())
	{
	}
};

/**
 * 절차적 레벨 생성을 위한 계층 구조 데이터를 담고 있는 데이터 애셋입니다.
 */
UCLASS(BlueprintType)
class ONEBUTTONLEVELGENERATION_API UOCGHierarchyDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 레벨 생성을 위한 계층 데이터의 배열입니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FLandscapeHierarchyData> HierarchyData;
};
