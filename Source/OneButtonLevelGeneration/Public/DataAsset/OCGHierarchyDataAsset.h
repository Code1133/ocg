// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "OCGHierarchyDataAsset.generated.h"


/**
 *
 */
USTRUCT(BlueprintType)
struct FLandscapeHierarchyData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName LayerName;
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
