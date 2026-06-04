// Copyright (c) 2025-2026 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OCGMapPresetSettings.generated.h"


// --- Erosion (입자 기반 수력 침식) ---
USTRUCT(BlueprintType)
struct FOCGErosionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	bool bErosion = true;

	// More Iteration gives more erosion details
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "1", ClampMax = "1000000")
	)
	int32 NumErosionIterations = 100000;

	// Decides the size of erosion
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "2", ClampMax = "8")
	)
	int32 ErosionRadius = 3;

	// Larger Inertia gives more smooth flow of erosion droplets
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "0.99")
	)
	float DropletInertia = 0.25f; // 1에 가까울 수록 직진 성향 강해짐 0에 가까울수록 기울기에 따른 무작위 움직임

	// Decides the capacity of sediment one droplet can have
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "100.0")
	)
	float SedimentCapacityFactor = 10.0f; // 흙 운반 용량 계수

	// Decides the minimum capacity of sediment one droplet can have
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0")
	)
	float MinSedimentCapacity = 0.01f; // 최소 운반 용량

	// Decides the speed of erosion
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0")
	)
	float ErodeSpeed = 0.3f; // 침식 속도

	// Decides the speed of deposit
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0")
	)
	float DepositSpeed = 0.3f; // 퇴적 속도

	// Decides how fast the droplet evaporates
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0")
	)
	float EvaporateSpeed = 0.01f; // 증발 속도

	// Decides the gravity effect on droplets
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "100.0")
	)
	float Gravity = 9.8f;

	// Decides the maximum lifetime of droplets
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "512")
	)
	int32 MaxDropletLifetime = 50;

	// Decides the initial water volume of droplets
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "10.0")
	)
	float InitialWaterVolume = 0.5f;

	// Decides the initial speed of droplets
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "20.0")
	)
	float InitialSpeed = 2.0f;
};

// --- Temperature (기온 범위) ---
USTRUCT(BlueprintType)
struct FOCGTemperatureSettings
{
	GENERATED_BODY()

	// Landscapes Minimum Temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = -273.15))
	float MinTemp = -30.0f;

	// Landscapes Maximum Temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = -273.15))
	float MaxTemp = 80.0f;
};

// --- Humidity (습도) ---
USTRUCT(BlueprintType)
struct FOCGHumiditySettings
{
	GENERATED_BODY()

	// Decides the amount of humidity drop per distance from water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Humidity", meta = (ClampMin = "0.0"))
	float MoistureFalloffRate = 0.0005f;

	// Decides the amount of change in humidity caused by temperature
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Humidity",
		meta = (ClampMin = "0.0", ClampMax = "1.0")
	)
	float TemperatureInfluenceOnHumidity = 0.7f;
};

// --- Island (섬 형태) ---
USTRUCT(BlueprintType)
struct FOCGIslandSettings
{
	GENERATED_BODY()

	// Decides whether the landscape will be island or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island")
	bool bIsland = true;

	// Decides the sharpness of island edge and island's size
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Island",
		meta = (EditCondition = "bIsland", EditConditionHides, ClampMin = 0.1, ClampMax = 3.0)
	)
	float IslandFalloffExponent = 2.0f;

	// Decides irregularity of island shape
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Island",
		meta = (EditCondition = "bIsland", EditConditionHides, ClampMin = "0.0001", ClampMax = "0.05")
	)
	float IslandShapeNoiseScale = 0.0025f;

	// Decides irregularity of island edge
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Island",
		meta = (EditCondition = "bIsland", EditConditionHides, ClampMin = 0.0001)
	)
	float IslandShapeNoiseStrength = 0.5f;
};

// --- Biome Terrain Modification (바이옴별 지형 변형) ---
USTRUCT(BlueprintType)
struct FOCGBiomeTerrainSettings
{
	GENERATED_BODY()

	// Decides whether the Mountain Ratio of biomes will be applied or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome Terrain")
	bool bModifyTerrainByBiome = false;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Biome Terrain",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = 0.0f, ClampMax = 1.0f)
	)
	float PlainSmoothFactor = 1.0f;

	// Decides the frequency of details in Biome
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Biome Terrain",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = "0.0001", ClampMax = "0.05")
	)
	float BiomeNoiseScale = 0.01f;

	// Decides the height of details in Biome
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Biome Terrain",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = "0.0001", ClampMax = "1.0")
	)
	float BiomeNoiseAmplitude = 0.2f;

	// Larger radius gives smaller spike height difference at biome borders
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Biome Terrain",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = "0", ClampMax = "50")
	)
	int32 BiomeHeightBlendRadius = 5;
};
