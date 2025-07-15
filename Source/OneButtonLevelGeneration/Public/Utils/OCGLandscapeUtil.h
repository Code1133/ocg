// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class ALandscape;
/**
 * 
 */
class ONEBUTTONLEVELGENERATION_API OCGLandscapeUtil
{
public:
	OCGLandscapeUtil();
	~OCGLandscapeUtil();

	static void ExtractHeightMap(ALandscape* InLandscape, const FGuid InGuid, int32& OutWidth, int32& OutHeight, TArray<uint16>& OutHeightMap);

	static void AddWeightMap(ALandscape* InLandscape, int32 InLayerIndex, int32 Width, int32 Height, const TArray<uint16>& InHeightDiffMap, TArray<
	                           uint8>& OutOriginWeightMap);

	static void ApplyWeightMap(ALandscape* InLandscape, int32 InLayerIndex, const TArray<uint8>& WeightMap);

	static void MakeWeightMapFromHeightDiff(const TArray<uint16>& HeightDiff, TArray<uint8>& OutWeight);

	static void BlurWeightMap(const TArray<uint8>& InWeight, TArray<uint8>& OutWeight, int32 Width, int32 Height);

	static void GetOriginWeightMap(ALandscape* InLandscape, TArray<uint8>& OutWeight);
};
