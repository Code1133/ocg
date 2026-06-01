// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

namespace OCGMapDataUtils
{
	/** 레이어 인덱스로부터 "Layer{N}" 형식의 FName을 생성합니다 (0=Water, 1..N=Biomes). */
	[[nodiscard]] inline FName MakeLayerName(int32 LayerIndex)
	{
		return FName{ FString::Printf(TEXT("Layer%d"), LayerIndex) };
	}

	bool TextureToHeightArray(UTexture2D* Texture, TArray<uint16>& OutHeightArray);
	
	bool ImportMap(TArray<uint16>& OutMapData, FIntPoint& OutResolution, const FString& FilePath);

	UTexture2D* ImportTextureFromPNG(const FString& FileName);

	bool ExportMap(const TArray<uint8>& InMap, const FIntPoint& Resolution, const FString& FileName);

	bool ExportMap(const TArray<uint16>& InMap, const FIntPoint& Resolution, const FString& FileName);

	bool ExportMap(const TArray<FColor>& InMap, const FIntPoint& Resolution, const FString& FileName);

	bool GetImageResolution(FIntPoint& OutResolution, const FString& FilePath);
}
