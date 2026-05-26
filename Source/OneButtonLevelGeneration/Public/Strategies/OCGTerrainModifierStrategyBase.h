// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
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
	 * @param ZScale Landscape Z 스케일 ((MaxHeight - MinHeight) * 0.001953125f)
	 * @param ZOffset Landscape Z 오프셋
	 */
	virtual void ModifyTerrainByBiome(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer, float ZScale, float ZOffset) PURE_VIRTUAL(UOCGTerrainModifierStrategyBase::ModifyTerrainByBiome);
};
