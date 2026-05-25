// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGSmoothingStrategyBase.generated.h"

class UMapPreset;

/**
 * 높이맵 평탄화(Smoothing) 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 가우시안 블러, 중앙값 필터, 경사도 기반 스파이크 제거 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGSmoothingStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 높이맵에 평탄화 패스를 적용하고 결과를 업데이트합니다.
	 * MapPreset.bSmoothHeight가 false이면 구현체는 즉시 반환해야 합니다.
	 *
	 * @param Preset 평탄화 파라미터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 원본 높이맵 데이터를 읽어오고, 평탄화된 결과를 다시 기록할 데이터 컨테이너
	 */
	virtual void SmoothHeightMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGSmoothingStrategyBase::SmoothHeightMap);
};
