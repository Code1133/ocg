// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGBiomeStrategyBase.generated.h"

class UMapPreset;

/**
 * 바이옴 결정 및 블렌딩 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 온도, 습도 기반 분류나 보로노이 분할 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGBiomeStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 바이옴 가중치 레이어를 계산하여 DataContainer에 기록합니다.
	 *
	 * @param Preset 바이옴 설정 데이터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 온도, 습도맵 데이터를 읽어오고, 계산된 바이옴 결과를 다시 기록할 데이터 컨테이너
	 */
	virtual void DecideAndBlendBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGBiomeStrategyBase::DecideAndBlendBiomes);

	/**
	 * Smoothing/Erosion 이후 높이맵 변화로 달라진 픽셀의 바이옴을 재분류하고 가중치 레이어를 재블렌딩합니다.
	 * bContainWater가 false이면 무시합니다.
	 *
	 * @param Preset 바이옴 설정 데이터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 갱신된 높이맵과 기존 바이옴 레이어가 담긴 컨테이너
	 */
	virtual void FinalizeBiomes(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGBiomeStrategyBase::FinalizeBiomes);
};
