// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGHeightmapStrategyBase.generated.h"

class UMapPreset;

/**
 * 높이맵 생성 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 노이즈 기반, 외부 파일 기반, 절차적 생성 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGHeightmapStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 지정된 프리셋 설정을 기반으로 높이맵 데이터를 생성하여 DataContainer에 기록합니다.
	 *
	 * @param Preset 지형 생성 파라미터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer 생성된 높이맵 결과물이 저장될 런타임 데이터 컨테이너
	 */
	virtual void GenerateHeightMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGHeightmapStrategyBase::GenerateHeightMap);
};
