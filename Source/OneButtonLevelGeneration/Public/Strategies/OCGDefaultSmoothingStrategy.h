// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGHeightConverter.h"
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
	/** 높이 변환기(HeightConverter)를 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

	/**
	 * 분리형(Horizontal -> Vertical) 가우시안 블러를 InOutHeightMap에 인플레이스로 적용합니다.
	 */
	void ApplyGaussianBlur(const UMapPreset* Preset, TArray<uint16>& InOutHeightMap);

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

private:
	/** 높이맵 uint16 <-> 월드 높이(cm) 변환기 (ZScale/ZOffset 보유) */
	FOCGHeightConverter HeightConverter;
};
