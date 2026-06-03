// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGFoliageStrategyBase.generated.h"

class UMapPreset;

/**
 * 폴리지 배치 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 포아송 디스크 샘플링, 격자 기반 배치 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGFoliageStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 바이옴 가중치 레이어를 기반으로 폴리지를 배치하고 생성된 데이터를 기록합니다.
	 *
	 * @param Preset 폴리지 밀도 및 배치 설정이 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 배치 계산에 필요한 지형 및 바이옴 레이어 데이터를 읽어올 런타임 데이터 컨테이너
	 */
	virtual void PlaceFoliage(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGFoliageStrategyBase::PlaceFoliage);
};
