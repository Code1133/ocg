// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"

class UMapPreset;

/**
 * 높이맵 uint16 값과 월드 높이(cm) 사이의 변환 및 관련 상수(ZScale/ZOffset)를 캡슐화하는 경량 헬퍼
 */
struct FOCGHeightConverter
{
	/** 높이맵 uint16 값을 월드 높이(cm)로 변환할 때 사용하는 Z축 스케일 계수 */
	float ZScale = 0.0f;

	/** 최대/최소 높이의 절댓값 차이를 보정하기 위한 Z축 오프셋 */
	float ZOffset = 0.0f;

public:
	/** Preset의 MinHeight/MaxHeight로부터 ZScale/ZOffset을 계산해 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

	/** uint16 높이맵 값을 실제 월드 높이(cm)로 변환합니다. */
	[[nodiscard]] FORCEINLINE float ToWorldHeight(const uint16 HeightMapValue) const
	{
		return (HeightMapValue - 32768.0f) * ZScale / 128.0f + ZOffset;
	}

	/** 실제 월드 높이(cm)를 uint16 높이맵 값으로 변환합니다. */
	[[nodiscard]] FORCEINLINE uint16 ToHeightMapValue(const float WorldHeight) const
	{
		return static_cast<uint16>((WorldHeight - ZOffset) * 128.0f / ZScale + 32768.0f);
	}

	/**
	 * Preset 설정 기준의 월드 해수면 높이(cm)를 반환합니다.
	 * 물이 없으면(bContainWater == false) MinHeight를 반환합니다.
	 */
	[[nodiscard]] static float GetSeaLevelWorldHeight(const UMapPreset* Preset);
};
