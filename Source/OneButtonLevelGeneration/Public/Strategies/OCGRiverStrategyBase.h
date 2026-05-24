// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGRiverStrategyBase.generated.h"

class UMapPreset;

/**
 * 강 경로 탐색 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 흐름 방향(Flow Direction) 기반, A* 경로 탐색 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGRiverStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 강 경로 데이터를 생성하여 DataContainer에 기록합니다.
	 *
	 * @param Preset 강 생성 파라미터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 높이맵 데이터를 읽어오고, 계산된 강 생성 결과를 다시 기록할 데이터 컨테이너
	 */
	virtual void GenerateRivers(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGRiverStrategyBase::GenerateRivers);
};
