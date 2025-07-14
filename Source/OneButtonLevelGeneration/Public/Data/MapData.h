// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "MapData.generated.h"

USTRUCT(BlueprintType)
struct FMapData
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<uint16> HeightMap;

	UPROPERTY()
	TArray<float> TemperatureMap;

	UPROPERTY()
	TArray<float> HumidityMap;
};

namespace OCGMapDataUtils
{
	bool TextureToHeightArray(UTexture2D* Texture, TArray<uint16>& OutHeightArray);
	
	bool ImportMap(TArray<uint16>& OutMapData, FIntPoint& OutResolution, const FString& FileName);

	UTexture2D* ImportTextureFromPNG(const FString& FileName);

	bool ExportMap(const TArray<uint8>& InMap, const FIntPoint& Resolution, const FString& FileName);

	bool ExportMap(const TArray<uint16>& InMap, const FIntPoint& Resolution, const FString& FileName);

	bool ExportMap(const TArray<FColor>& InMap, const FIntPoint& Resolution, const FString& FileName);
}
