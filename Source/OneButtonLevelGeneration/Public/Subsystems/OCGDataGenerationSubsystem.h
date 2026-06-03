// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Data/OCGWorldDataContainer.h"
#include "OCGDataGenerationSubsystem.generated.h"

class UMapPreset;
class UOCGHeightmapStrategyBase;
class UOCGTemperatureStrategyBase;
class UOCGHumidityStrategyBase;
class UOCGBiomeStrategyBase;
class UOCGTerrainModifierStrategyBase;
class UOCGErosionStrategyBase;
class UOCGSmoothingStrategyBase;

/**
 * 높이맵, 온도맵, 습도맵 생성과 바이옴 배정, 평탄화(Smoothing) 및 침식 데이터 처리를 담당하는 에디터 서브시스템
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UOCGDataGenerationSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Preset을 기반으로 전체 맵 데이터를 생성합니다.
	 * 호출 시 DataContainer가 초기화된 후 각 전략(Strategy) 인터페이스가 순서대로 실행됩니다.
	 */
	void GenerateData(const UMapPreset* Preset);

	/** 생성된 맵 데이터 컨테이너에 대한 읽기 전용 참조를 반환합니다. */
	FORCEINLINE const FOCGWorldDataContainer& GetDataContainer() const { return DataContainer; }

	/** 생성된 맵 데이터 컨테이너에 대한 쓰기 가능 참조를 반환합니다. ApplyLandscape 파이프라인에서 in-place 수정 시 사용합니다. */
	FORCEINLINE FOCGWorldDataContainer& GetDataContainer() { return DataContainer; }

private:
	/**
	 * 전체 파이프라인 완료 후 HeightMapData를 순회하여 CurMinHeight / CurMaxHeight를 계산합니다.
	 * 기존 UOCGMapGenerateComponent::GetMaxMinHeight()에 해당합니다.
	 */
	void ComputeHeightRange(const UMapPreset* Preset);

private:
	/** 생성 파이프라인의 중간 결과물을 보관하는 런타임 전용 컨테이너 */
	FOCGWorldDataContainer DataContainer;

	/** 높이맵 생성 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGHeightmapStrategyBase> HeightmapStrategy;

	/** 온도맵 생성 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGTemperatureStrategyBase> TemperatureStrategy;

	/** 습도맵 생성 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGHumidityStrategyBase> HumidityStrategy;

	/** 바이옴 결정 및 블렌딩 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGBiomeStrategyBase> BiomeStrategy;

	/** 바이옴 기반 지형 수정 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGTerrainModifierStrategyBase> TerrainModifierStrategy;

	/** 수력 침식 시뮬레이션 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGErosionStrategyBase> ErosionStrategy;

	/** 높이맵 평탄화 전략 구현체 */
	UPROPERTY()
	TObjectPtr<UOCGSmoothingStrategyBase> SmoothingStrategy;
};
