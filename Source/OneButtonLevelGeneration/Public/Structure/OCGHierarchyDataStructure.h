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
	UPROPERTY(BlueprintReadOnly, Category = "OCG")
	FName MeshFilterName_Internal;
};

/**
 * Defines slope angle limits in degrees for mesh generation. Retrieves meshes within the Min to Max angle range.
 */
USTRUCT(BlueprintType)
struct FSlopeLimitInfo
{
	GENERATED_BODY()

	/** The minimum slope angle in degrees (0-90). */
	UPROPERTY(
		EditAnywhere, BlueprintReadOnly, Category = "OCG",
		meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 90.0f, UIMax = 90.0f, Units = "Degrees")
	)
	float MinAngle = 0.0f;

	/** The maximum slope angle in degrees (0-90). */
	UPROPERTY(
		EditAnywhere, BlueprintReadOnly, Category = "OCG",
		meta = (ClampMin = 0.0f, UIMin = 0.0f, ClampMax = 90.0f, UIMax = 90.0f, Units = "Degrees")
	)
	float MaxAngle = 45.0f;
};

/**
 * A structure that defines transform (offset, rotation, scale) information for a point.
 */
USTRUCT(BlueprintType)
struct FTransformPointInfo
{
	GENERATED_BODY()

	/** Minimum offset to apply to the point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FVector OffsetMin = FVector::ZeroVector;

	/** Maximum offset to apply to the point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FVector OffsetMax = FVector::ZeroVector;

	/** If true, the offset is an absolute value, otherwise it's relative to the point's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	bool bAbsoluteOffset = false;

	/** Minimum rotation to apply to the point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FRotator RotationMin = FRotator::ZeroRotator;

	/** Maximum rotation to apply to the point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FRotator RotationMax = FRotator(0.0f, 360.0f, 0.0f);

	/** If true, the rotation is an absolute value, otherwise it's relative to the point's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	bool bAbsoluteRotation = false;

	/** Minimum scale to apply to the point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FVector ScaleMin = FVector::OneVector;

	/** Maximum scale to apply to the point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	FVector ScaleMax = FVector::OneVector;

	/** If true, the scale is an absolute value, otherwise it's relative to the point's transform. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	bool bAbsoluteScale = false;
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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, DisplayName = "Selected Landscape Layer Name", Category = "OCG")
	FName LayerName_Internal;

	/** Internal name used when filtering Meshes in PCG. */
	UPROPERTY(BlueprintReadOnly, Category = "OCG")
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

	/** Whether to override the transform point settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (InlineEditConditionToggle))
	bool bOverrideTransformPoint = false;

	/** The transform point information to use when overriding. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG", meta = (EditCondition = "bOverrideTransformPoint"))
	FTransformPointInfo TransformPoint;

	/** Whether to prune (remove) meshes that overlap with each other. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	bool bPruningOverlappedMeshes = true;

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DisplayName = "Point Debug Color", Category = "OCG|Debug", AdvancedDisplay)
	FLinearColor DebugColorLinear = FLinearColor::White;

	FLandscapeHierarchyData()
		: Seed(FMath::Rand())
	{
	}
};
