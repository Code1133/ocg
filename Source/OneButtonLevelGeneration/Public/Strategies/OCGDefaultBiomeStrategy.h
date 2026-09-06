// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGBiomeStrategyBase.h"
#include "OCGDefaultBiomeStrategy.generated.h"

struct FOCGBiomeSettings;

/**
 * 온도/습도 기반 최소 거리(MinDist) 메트릭과 분리형(Separable) 박스 블러로 바이옴 가중치 레이어를 생성하는 기본 Biome 전략 구현체.
 * OCGMapGenerateComponent의 DecideBiome + BlendBiome 로직을 Strategy 인터페이스에 맞게 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultBiomeStrategy : public UOCGBiomeStrategyBase
{
	GENERATED_BODY()

public:
	/**
	 * 온도/습도 맵을 참조하여 픽셀별 바이옴을 결정(Decide)하고, 분리형 박스 블러로 가중치 레이어를 블렌딩합니다.
	 * 결과는 DataContainer.WeightLayers(블렌딩된 가중치)와 DataContainer.BiomeLayerMap(픽셀별 레이어 인덱스)에 기록됩니다.
	 */
	virtual void DecideAndBlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

	/**
	 * Smoothing/Erosion 이후 해수면 기준으로 변화한 픽셀의 바이옴을 재분류하고 가중치 레이어를 재블렌딩합니다.
	 * bContainWater가 false이면 즉시 반환합니다.
	 */
	virtual void FinalizeBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/**
	 * BiomeNameMap을 기반으로 Horizontal -> Vertical 분리형 박스 블러를 수행합니다.
	 * 블렌딩 후 픽셀 단위 가중치 합이 255가 되도록 정규화합니다.
	 */
	void BlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer);

	/**
	 * 한 픽셀의 온도/습도를 기반으로 최근접 바이옴을 결정합니다.
	 * DecideAndBlendBiomes와 FinalizeBiomes가 공유하는 분류 로직입니다.
	 * @param Preset MapPreset 인스턴스
	 * @param DataContainer 임시 데이터를 저장할 컨테이너
	 * @param Index 픽셀 인덱스
	 * @param TotalWeight 전체 바이옴 Weight 합 (거리 메트릭 정규화용)
	 * @param SeaLevelHeight 해수면 높이(heightmap uint16). Height가 이보다 낮으면 Water(0)로 분류
	 * @param OutBiome 결정된 바이옴 설정 포인터 (미결정 시 nullptr)
	 * @return 레이어 인덱스 (0=Water, 1..N=Biomes[N-1], 미결정 시 INDEX_NONE)
	 */
	[[nodiscard]] uint32 FindBiomeForPixel(
		const UMapPreset* Preset,
		const FOCGWorldDataContainer& DataContainer,
		int32 Index,
		float TotalWeight,
		uint16 SeaLevelHeight,
		const FOCGBiomeSettings*& OutBiome
	) const;

	/** 모든 바이옴의 Weight 합을 반환합니다. */
	[[nodiscard]] float ComputeTotalBiomeWeight(const UMapPreset* Preset) const;

private:
	/** DecideAndBlendBiomes에서 픽셀별 배정된 레이어 이름을 기록합니다. BlendBiomes가 소스 맵으로 참조합니다. */
	TArray<FName> BiomeNameMap;
};
