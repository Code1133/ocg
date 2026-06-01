// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Data/OCGHeightConverter.h"
#include "Strategies/OCGErosionStrategyBase.h"
#include "OCGDefaultErosionStrategy.generated.h"

/**
 * 입자 기반(Particle-based) 수력 침식 시뮬레이션으로 높이맵을 침식하는 기본 Erosion 전략 구현체.
 * OCGMapGenerateComponent의 기존 ErosionPass, InitializeErosionBrush, CalculateHeightAndGradient 로직을
 * Strategy 인터페이스에 맞춰 이식한 클래스입니다.
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDefaultErosionStrategy : public UOCGErosionStrategyBase
{
	GENERATED_BODY()

public:
	/**
	 * 사전 계산된 침식 브러시를 활용하여 물방울(Droplet) 시뮬레이션으로 높이맵을 침식합니다.
	 * MapPreset.bErosion이 false이거나 NumErosionIterations가 0이면 즉시 반환합니다.
	 * 결과는 DataContainer.HeightMapData에 인플레이스(In-place)로 기록됩니다.
	 */
	virtual void ApplyErosion(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;

private:
	/** FRandomStream과 높이 변환기(HeightConverter)를 초기화합니다. */
	void Initialize(const UMapPreset* Preset);

	/**
	 * 각 픽셀에 대한 침식 브러시(반경 내 인덱스, 가중치 배열)를 사전 계산합니다.
	 * ErosionRadius나 MapResolution이 변경될 때만 재계산합니다.
	 */
	void InitializeErosionBrush(const UMapPreset* Preset);

	/**
	 * 주어진 좌표에서 높이와 기울기(Gradient)를 이중선형 보간(Bilinear Interpolation)으로 계산합니다.
	 * @return 보간된 월드 높이(cm)
	 */
	[[nodiscard]] float CalculateHeightAndGradient(
		const UMapPreset* Preset, const TArray<float>& HeightMap, float LandscapeScale, float PosX, float PosY, FVector2D& OutGradient) const;

private:
	/** 물방울 시뮬레이션에 사용되는 난수 생성기 (Preset->Seed로 초기화) */
	FRandomStream Stream;

	/** 높이맵 uint16 <-> 월드 높이(cm) 변환기 (ZScale/ZOffset 보유) */
	FOCGHeightConverter HeightConverter;

	/** 마지막으로 브러시를 계산했을 때의 ErosionRadius. 변경 감지에 사용됩니다. */
	int32 CachedErosionRadius = -1;

	/** 픽셀별 침식 브러시 범위 내 이웃 픽셀 인덱스 배열 */
	TArray<TArray<int32>> ErosionBrushIndices;

	/** 픽셀별 침식 브러시 범위 내 이웃 픽셀 가중치 배열 (합산 = 1.0) */
	TArray<TArray<float>> ErosionBrushWeights;
};
