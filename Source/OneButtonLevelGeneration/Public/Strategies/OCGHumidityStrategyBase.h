// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGHumidityStrategyBase.generated.h"

class UMapPreset;

/**
 * 습도맵 생성 알고리즘을 유연하게 교체할 수 있도록 추상화한 전략(Strategy) 인터페이스
 * 이 인터페이스를 상속받아 BFS 기반, 노이즈 기반, 외부 데이터 기반 등 다양한 방식의 구현체로 확장할 수 있습니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGHumidityStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 높이맵과 온도맵 데이터를 참조하여 습도맵을 생성하고 DataContainer에 기록합니다.
	 * 생성 후 전역 최솟값/최댓값은 DataContainer.MinHumidity / MaxHumidity에 저장됩니다.
	 *
	 * @param Preset 습도 파라미터가 포함된 UMapPreset 에셋 설정 데이터
	 * @param DataContainer HeightMapData와 TemperatureMapData를 읽어오고, 생성된 습도맵 결과를 기록할 데이터 컨테이너
	 */
	virtual void GenerateHumidityMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) PURE_VIRTUAL(UOCGHumidityStrategyBase::GenerateHumidityMap);
};
