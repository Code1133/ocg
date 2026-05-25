// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGBiomeStrategyBase.h"
#include "OCGDefaultBiomeStrategy.generated.h"

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
	 * 결과는 DataContainer.WeightLayers(블렌딩된 가중치)와 DataContainer.BiomeMap(픽셀별 바이옴 포인터)에 기록됩니다.
	 */
	virtual void DecideAndBlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/**
	 * BiomeNameMap을 기반으로 Horizontal -> Vertical 분리형 박스 블러를 수행합니다.
	 * 블렌딩 후 픽셀 단위 가중치 합이 255가 되도록 정규화합니다.
	 */
	void BlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer);

private:
	/** DecideAndBlendBiomes에서 픽셀별 배정된 레이어 이름을 기록합니다. BlendBiomes가 소스 맵으로 참조합니다. */
	TArray<FName> BiomeNameMap;
};
