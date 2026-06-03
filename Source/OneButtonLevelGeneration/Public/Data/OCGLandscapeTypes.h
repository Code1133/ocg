// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "OCGLandscapeTypes.generated.h"

/**
 * OCG 플러그인의 공용 지형 구조체 정의 구조체
 */
USTRUCT(BlueprintType)
struct FLandscapeSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 WorldPartitionGridSize = 2;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 WorldPartitionRegionSize = 16;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	uint32 QuadsPerSection = 0;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	FIntPoint TotalLandscapeComponentSize = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 ComponentCountX = 0;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 ComponentCountY = 0;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 QuadsPerComponent = 0;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 SizeX = 0;

	UPROPERTY(EditAnywhere, Category = "Landscape|Cache")
	int32 SizeY = 0;

	[[nodiscard]] bool operator==(FLandscapeSetting const& Other) const = default;
};
