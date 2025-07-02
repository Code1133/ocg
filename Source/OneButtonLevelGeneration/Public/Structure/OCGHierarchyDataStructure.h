// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OCGHierarchyDataStructure.generated.h"


/**
 * A structure that defines a mesh and its spawn weight.
 */
USTRUCT(BlueprintType)
struct FOCGMeshInfo
{
	GENERATED_BODY()

	/** Static mesh to place. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	TObjectPtr<UStaticMesh> Mesh;

	/** Mesh spawn weight. The probability this mesh will be selected compared to other meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (ClampMin = 0, UIMin = 0))
	int32 Weight = 1;

	/** Enable default collision for the mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	bool bEnableDefaultCollision = false;

	/** The layer name this mesh belongs to. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OCG")
	FName MeshFilterName_Internal;
};

USTRUCT(BlueprintType)
struct FSlopeLimitInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 1.0f, UIMax = 1.0f))
	float LowerLimit = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 1.0f, UIMax = 1.0f))
	float UpperLimit = 1.0f;
};

/**
 * A structure that defines hierarchical generation data for a specific landscape layer.
 */
USTRUCT(BlueprintType)
struct FLandscapeHierarchyData
{
	GENERATED_BODY()

	/** The name of the layer that this hierarchy data represents. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FName BiomeName;

	/** Internal name used when matching Landscape Layer in PCG. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OCG")
	FName LayerName_Internal;

	/** Internal name used when filtering Meshes in PCG. */
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly, Category = "OCG")
	FName MeshFilterName_Internal;

	/** Weight of the blended layer. Values between 0 and 1, higher values increase this layer's influence. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float BlendingRatio = 0.5f;

	/** Number of points to generate per square meter. Determines point density. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	float PointsPerSquareMeter = 0.1f;

	/** Influence range (radius) of each point along X, Y, Z axes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FVector PointExtent = FVector{100.0f, 100.0f, 100.0f};

	/** Degree of irregularity or 'looseness' in point placement. Higher values spread points more widely. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	float Looseness = 1.0f;

	/** Controls point steepness. Values between 0 and 1, closer to 1 means steeper slopes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f))
	float PointSteepness = 0.5f;

	/** Whether to generate Mesh on slopes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (InlineEditConditionToggle))
	bool bSlopeLimit = false;

	/** Slope limit information used when generating Mesh on slopes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (ClampMin = 0.0f, ClampMax = 1.0f, UIMin = 0.0f, UIMax = 1.0f, EditCondition = "bSlopeLimit"))
	FSlopeLimitInfo SlopeLimits;

	/** Distance at which world position offset gets disabled. 0 means always enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG|Optimization")
	int32 WorldPositionOffsetDisableDistance = 0;

	/** Start distance for mesh culling. 0 means always enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG|Optimization")
	int32 StartCullDistance = 0;

	/** End distance for mesh culling. 0 means always enabled. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG|Optimization")
	int32 EndCullDistance = 0;

	/** Sets Affect Distance Field Lighting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG|Optimization")
	bool bAffectDistanceFieldLighting = true;

	/**
	 * Whether to process Static Mesh generation on the GPU.
	 * @warning Mesh processed on GPU will not work with Culling and Collision features.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	bool bExecuteOnGPU = false;

	/** Random seed for point generation. Same seed produces same results. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	int32 Seed;

	/** Array of meshes and weights to be placed in this layer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	TArray<FOCGMeshInfo> Meshes;

	/** Color used to visualize this layer during debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG|Debug", AdvancedDisplay)
	FLinearColor DebugColor = FLinearColor::White;

	FLandscapeHierarchyData()
		: Seed(FMath::Rand())
	{
	}
};
