// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OCGHierarchyDataAsset.generated.h"


/**
 *
 */
USTRUCT(BlueprintType)
struct FMeshAndWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0, UIMin = 0))
	int32 Weight = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LayerName;
};

/**
 *
 */
USTRUCT(BlueprintType)
struct FLandscapeHierarchyData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float BlendedWeight = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float PointsPerSquareMeter = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector PointExtent = FVector{100.0f, 100.0f, 100.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Looseness = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float PointSteepness = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Seed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor DebugColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMeshAndWeight> Meshes;

	FLandscapeHierarchyData()
		: Seed(FMath::Rand())
	{
	}
};

/**
 *
 */
UCLASS(BlueprintType)
class ONEBUTTONLEVELGENERATION_API UOCGHierarchyDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FLandscapeHierarchyData> HierarchyData;
};
