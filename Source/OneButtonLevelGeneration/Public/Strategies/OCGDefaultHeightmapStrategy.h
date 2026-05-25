// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGHeightmapStrategyBase.h"
#include "OCGDefaultHeightmapStrategy.generated.h"

/**
 * Perlin noise + island mask 조합으로 높이맵을 생성하는 기본 HeightMap 전략 구현체.
 * OCGMapGenerateComponent의 기존 높이맵 생성 로직을 Strategy 인터페이스에 맞춰 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultHeightmapStrategy : public UOCGHeightmapStrategyBase
{
	GENERATED_BODY()

public:
	/**
	 * Preset 설정값을 기반으로 높이맵을 생성하여 DataContainer.HeightMapData에 기록합니다.
	 * 호출 시 내부 노이즈 오프셋이 Preset->Seed로 재초기화되므로, 동일한 Preset으로 여러 번 호출해도 항상 같은 결과를 반환합니다.
	 */
	virtual void GenerateHeightMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/** Seed, 노이즈 스케일, PlainHeight 등 생성에 필요한 내부 상태를 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

	/** 각 노이즈 레이어(산, 평야, 혼합, 디테일, 섬)의 랜덤 오프셋을 Stream에서 추출하여 설정합니다. */
	void InitializeNoiseOffsets(const UMapPreset* Preset);

	/** 지정한 좌표 (InX, InY)의 높이값을 0~1 범위로 계산하여 반환합니다. */
	[[nodiscard]] float CalculateHeightForCoordinate(const UMapPreset* Preset, int32 InX, int32 InY) const;

private:
	/** Seed 기반 난수 생성기 */
	FRandomStream Stream;

	/**
	 * LandscapeScale이 노이즈에 적용될 때의 보정 계수.
	 * @note ApplyScaleToNoise가 false이면 1.0 고정.
	 */
	float NoiseScale = 1.0f;

	/** 물이 있을 경우 해수면 바로 위, 없을 경우 0으로 설정되는 평야 기준 높이 (0~1 정규화 값) */
	float PlainHeight = 0.0f;

	/** 평야 노이즈 레이어의 공간 오프셋 (Seed별로 고유한 값을 가집니다.) */
	FVector2D PlainNoiseOffset;

	/** 대륙/산악 노이즈 레이어의 공간 오프셋. */
	FVector2D MountainNoiseOffset;

	/** 평야-산악 블렌딩 마스크 노이즈의 공간 오프셋. */
	FVector2D BlendNoiseOffset;

	/** 지형의 미세한 디테일 노이즈(옥타브 누적)를 위한 공간 오프셋. */
	FVector2D DetailNoiseOffset;

	/** 섬 외곽선 노이즈의 공간 오프셋. bIsland가 false이면 사용되지 않습니다. */
	FVector2D IslandNoiseOffset;
};
