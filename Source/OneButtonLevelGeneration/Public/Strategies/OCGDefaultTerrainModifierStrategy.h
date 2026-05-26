// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGTerrainModifierStrategyBase.h"
#include "OCGDefaultTerrainModifierStrategy.generated.h"

/**
 * 바이옴 레이어 인덱스(BiomeLayerMap)를 기반으로 높이맵을 수정하는 기본 TerrainModifier 전략 구현체.
 * OCGMapGenerateComponent::ModifyLandscapeWithBiome 로직을 Strategy 인터페이스에 맞게 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultTerrainModifierStrategy : public UOCGTerrainModifierStrategyBase
{
	GENERATED_BODY()

public:
	virtual void ModifyTerrainByBiome(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer, float ZScale, float ZOffset) override;

private:
	/**
	 * 각 바이옴 연결 영역의 최소 높이를 계산합니다.
	 * @note 원본: UOCGMapGenerateComponent::CalculateBiomeMinHeights
	 */
	void CalculateBiomeMinHeights(const UMapPreset* Preset, const TArray<uint16>& InHeightMap, const TArray<int32>& InBiomeLayerMap, TArray<float>& OutMinHeights, float ZScale, float ZOffset);

	/**
	 * 바이옴 최소 높이 배열에 슬라이딩 윈도우 박스 블러를 적용합니다.
	 * @note 원본: UOCGMapGenerateComponent::BlurBiomeMinHeights
	 */
	static void BlurBiomeMinHeights(const UMapPreset* Preset, const TArray<float>& InMinHeights, TArray<float>& OutMinHeights);

	/**
	 * BFS로 동일 바이옴 연결 영역을 탐색해 최소 높이를 구합니다.
	 * @note 원본: UOCGMapGenerateComponent::GetBiomeStats
	 */
	static void GetBiomeStats(FIntPoint MapSize, int32 X, int32 Y, int32 RegionID, float& OutMinHeight, TArray<int32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<int32>& InBiomeLayerMap, float ZScale, float ZOffset);
};
