// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGHeightConverter.h"
#include "Strategies/OCGHumidityStrategyBase.h"
#include "OCGDefaultHumidityStrategy.generated.h"

/**
 * BFS 기반 수분 거리 + 온도 보정으로 습도맵을 생성하는 기본 Humidity 전략 구현체.
 * OCGMapGenerateComponent의 GenerateHumidityMap 로직을 Strategy 인터페이스에 맞게 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultHumidityStrategy : public UOCGHumidityStrategyBase
{
	GENERATED_BODY()

public:
	/**
	 * HeightMapData와 TemperatureMapData를 참조하여 습도맵을 생성하고 DataContainer.HumidityMapData에 기록합니다.
	 * 생성된 전역 Min/MaxHumidity는 DataContainer.MinHumidity / MaxHumidity에 저장됩니다.
	 */
	virtual void GenerateHumidityMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/** 높이 변환기(HeightConverter)를 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

private:
	/** 높이맵 uint16 <-> 월드 높이(cm) 변환기 (ZScale/ZOffset 보유) */
	FOCGHeightConverter HeightConverter;
};
