// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"

/**
 * 맵 생성 파이프라인의 중간 결과물을 담는 런타임 전용 컨테이너입니다.
 * Subsystem 간 데이터 전달용으로만 사용하며, 직렬화(저장)되지 않습니다.
 *
 * v2 이전에는 이 데이터들이 UMapPreset에 혼재되어 있었으나,
 * 런타임 상태 데이터와 에셋 설정값을 명확히 분리하기 위해 도입되었습니다.
 */
struct ONEBUTTONLEVELGENERATION_API FOCGWorldDataContainer
{
	/** 생성된 높이맵 데이터 (uint16, MapResolution.X * MapResolution.Y 크기) */
	TArray<uint16> HeightMapData;

	/** 생성된 온도맵 데이터 */
	TArray<uint16> TemperatureMapData;

	/** 생성된 습도맵 데이터 */
	TArray<uint16> HumidityMapData;

	/** 바이옴별 Landscape 레이어 가중치 데이터 (레이어 이름 -> 가중치 배열) */
	TMap<FName, TArray<uint8>> WeightLayers;

	/**
	 * 픽셀별 배정된 바이옴 레이어 인덱스 (MapResolution.X * MapResolution.Y 크기)
	 * 0 = Water, 1..N = Preset->Biomes[N-1]. TerrainModifier/FinalizeBiomes가 참조합니다.
	 */
	TArray<int32> BiomeLayerMap;

	/**
	 * 온도맵 생성 후 계산된 전역 최솟값/최댓값 (실제 온도 단위, °C)
	 * TemperatureMapData의 uint16 값을 실제 온도로 역산할 때 사용하며, Biome 전략이 참조합니다.
	 */
	float MinTemp = 0.0f;
	float MaxTemp = 0.0f;

	/**
	 * 습도맵 생성 후 계산된 전역 최솟값/최댓값 (0~1 정규화 값)
	 * HumidityMapData의 uint16 값을 실제 습도로 역산할 때 사용하며, Biome 전략이 참조합니다.
	 */
	float MinHumidity = 0.0f;
	float MaxHumidity = 0.0f;

	/**
	 * 전체 생성 파이프라인 완료 후 계산된 실제 높이맵의 최댓값 (월드 높이, cm 단위)
	 * Hydrology가 강 발원 후보의 높이 임계값을 정할 때 참조합니다.
	 */
	float CurMaxHeight = 0.0f;

public:
	/** 모든 데이터 배열을 비우고 초기 상태로 되돌립니다. */
	void Reset();
};
