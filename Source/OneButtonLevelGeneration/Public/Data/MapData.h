// Fill out your copyright notice in the Description page of Project Settings.

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

namespace MapDataUtils
{
	bool TextureToHeightArray(UTexture2D* Texture, TArray<uint16>& OutHeightArray);
	
	bool ImportMap(TArray<uint16>& OutMapData, FIntPoint& OutResolution, const FString& FileName);

	UTexture2D* ImportTextureFromPNG(const FString& FileName);

	bool ExportMap(const TArray<uint16>& InMap, const FIntPoint& Resolution, const FString& FileName);

	bool ExportMap(const TArray<FColor>& InBiomeMap, const FIntPoint& Resolution, const FString& FileName);
}