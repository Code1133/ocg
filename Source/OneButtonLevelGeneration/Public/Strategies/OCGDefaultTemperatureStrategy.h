// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGHeightConverter.h"
#include "Strategies/OCGTemperatureStrategyBase.h"
#include "OCGDefaultTemperatureStrategy.generated.h"

/**
 * 고도 보정 Perlin noise 기반으로 온도맵을 생성하는 기본 Temperature 전략 구현체.
 * OCGMapGenerateComponent의 GenerateTempMap 로직을 Strategy 인터페이스에 맞게 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultTemperatureStrategy : public UOCGTemperatureStrategyBase
{
	GENERATED_BODY()

public:
	/**
	 * HeightMapData를 참조하여 온도맵을 생성하고 DataContainer.TemperatureMapData에 기록합니다.
	 * 생성된 전역 Min/MaxTemp는 DataContainer.MinTemp / MaxTemp에 저장됩니다.
	 */
	virtual void GenerateTemperatureMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/** Seed로부터 PlainNoiseOffset을 추출하고 높이 변환기(HeightConverter)를 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

private:
	/** 온도 노이즈에 사용되는 공간 오프셋 (컴포넌트의 PlainNoiseOffset과 동일한 스트림 위치에서 추출). */
	FVector2D PlainNoiseOffset;

	/** 높이맵 uint16 <-> 월드 높이(cm) 변환기 (ZScale/ZOffset 보유) */
	FOCGHeightConverter HeightConverter;
};
