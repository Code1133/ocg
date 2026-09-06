// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGErosionStrategyBase.generated.h"

class UMapPreset;

/**
 * 침식 시뮬레이션 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 수력 침식, 열 침식, GPU 기반 시뮬레이션 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGErosionStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 높이맵에 침식 패스를 적용하고 결과를 업데이트합니다.
	 *
	 * @param Preset 침식 파라미터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 원본 높이맵 데이터를 읽어오고, 침식이 적용된 지형 결과를 다시 기록할 데이터 컨테이너
	 */
	virtual void ApplyErosion(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGErosionStrategyBase::ApplyErosion);
};
