// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGHeightConverter.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGTerrainModifierStrategyBase.generated.h"

class UMapPreset;

/**
 * 바이옴 설정에 따라 높이맵을 수정하는 전략(Strategy) 인터페이스
 * DecideAndBlendBiomes 이후, SmoothHeightMap 이전에 호출됩니다.
 */
UCLASS(Abstract)
class ONEBUTTONLEVELGENERATION_API UOCGTerrainModifierStrategyBase : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 바이옴 설정(MountainRatio, PlainSmoothFactor 등)에 따라 HeightMapData를 In-place 수정합니다.
	 *
	 * @param Preset 생성 설정 에셋
	 * @param DataContainer 높이맵과 바이옴 레이어가 담긴 컨테이너 (HeightMapData In-place 수정됨)
	 * @param Converter 높이맵 uint16 <-> 월드 높이 변환기 (ZScale/ZOffset 보유)
	 */
	virtual void ModifyTerrainByBiome(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer, const FOCGHeightConverter& Converter) PURE_VIRTUAL(UOCGTerrainModifierStrategyBase::ModifyTerrainByBiome);
};
