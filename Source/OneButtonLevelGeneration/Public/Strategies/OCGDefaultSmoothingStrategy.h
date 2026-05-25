// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGSmoothingStrategyBase.h"
#include "OCGDefaultSmoothingStrategy.generated.h"

/**
 * 스파이크 제거(ApplySpikeSmooth) -> 가우시안 블러(ApplyGaussianBlur) -> 중앙값 필터(MedianSmooth) 순서로 높이맵을 평탄화하는 기본 Smoothing 전략 구현체.
 * OCGMapGenerateComponent의 SmoothHeightMap 및 관련 헬퍼 로직을 Strategy 인터페이스에 맞춰 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultSmoothingStrategy : public UOCGSmoothingStrategyBase
{
	GENERATED_BODY()

public:
	/**
	 * MapPreset 설정에 따라 스파이크 제거 -> 가우시안 블러 -> 중앙값 필터를 순서대로 적용합니다.
	 * MapPreset.bSmoothHeight가 false이면 즉시 반환합니다.
	 * 결과는 DataContainer.HeightMapData에 인플레이스(In-place)로 기록됩니다.
	 */
	virtual void SmoothHeightMap(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/** LandscapeZScale, ZOffset 등 높이 변환 상수를 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

	/**
	 * 분리형(Horizontal -> Vertical) 가우시안 블러를 적용합니다.
	 * 결과는 OutBlurredMap에 기록되며, InOutHeightMap에 다시 덮어씁니다.
	 */
	void ApplyGaussianBlur(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap, TArray<uint16>& OutBlurredMap);

	/**
	 * 경사도(Slope)가 MaxSlopeAngle을 초과하는 영역을 반복적으로 평탄화합니다.
	 * MapPreset.bSmoothBySlope가 false이면 즉시 반환합니다.
	 */
	void ApplySpikeSmooth(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap);

	/**
	 * 단일 커널 영역의 경사도를 검사하고, 초과 시 보정된 평면으로 높이를 재계산합니다.
	 * ApplySpikeSmooth의 내부 루프에서 픽셀별로 호출됩니다.
	 */
	void ProcessPlane(
	    const UMapPreset* Preset,
	    int32 CenterX,
	    int32 CenterY,
	    FIntPoint MapSize,
		int32 KernelRadius,
		int32 KernelSize,
		float MaxAllowedSlope,
		int32& SmoothedRegion,
		const TArray<uint16>& InOriginalHeightMap,
		TArray<uint16>& OutHeightMap
    );

	/**
	 * 중앙값 필터(Median Filter)를 적용하여 고립된 노이즈 픽셀을 제거합니다.
	 * MapPreset.bSmoothByMediumHeight가 false이면 즉시 반환합니다.
	 */
	void MedianSmooth(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap);

	/** uint16 높이맵 값을 실제 월드 높이(cm)로 변환합니다. */
	float HeightMapToWorldHeight(uint16 Height) const;

	/** 실제 월드 높이(cm)를 uint16 높이맵 값으로 변환합니다. */
	uint16 WorldHeightToHeightMap(float Height) const;

private:
	/** 높이맵 uint16 -> 월드 높이(cm) 변환 시 사용되는 스케일 계수 */
	float LandscapeZScale = 0.0f;

	/** 최대/최소 높이의 절댓값 차이를 보정하기 위한 Z축 오프셋 */
	float ZOffset = 0.0f;
};
