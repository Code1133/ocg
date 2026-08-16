// Copyright (c) 2025-2026 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "OCGMapPresetSettings.generated.h"

class UMaterialInterface;
class UMaterialInstance;
class UCurveFloat;

// 7, 15, 31, 63, 127, 255만 선택 가능한 열거형
UENUM(BlueprintType)
enum class ELandscapeQuadsPerSection : uint8
{
	Q0	 = 0	UMETA(DisplayName = "0"),
	Q7   = 7    UMETA(DisplayName = "7"),
	Q15  = 15   UMETA(DisplayName = "15"),
	Q31  = 31   UMETA(DisplayName = "31"),
	Q63  = 63   UMETA(DisplayName = "63"),
	Q127 = 127  UMETA(DisplayName = "127"),
	Q255 = 255  UMETA(DisplayName = "255"),
};

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

// --- Advanced Temperature ---
USTRUCT(BlueprintType)
struct FOCGAdvancedTemperatureSettings
{
	GENERATED_BODY()

	// Decides the amount of temperature drop per 1000 units of height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Temperature", meta = (ClampMin = "0.0"))
	float TempDropPer1000Units = 0.1f;
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

// --- Height (높이 범위 / 해수면) ---
USTRUCT(BlueprintType)
struct FOCGHeightSettings
{
	GENERATED_BODY()

	// Landscapes Minimum Height (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height")
	float MinHeight = -15000.0f;

	// Landscapes Maximum Height (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height")
	float MaxHeight = 20000.0f;

	// Decides the sea level height of landscape 0(Minimum height) ~ 1(Maximum height)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Height", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SeaLevel = 0.4f;
};

// --- Basic Noise (기본 노이즈 스케일) ---
USTRUCT(BlueprintType)
struct FOCGBasicNoiseSettings
{
	GENERATED_BODY()

	// Decides the frequency of Mountains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0001", ClampMax = "0.005"))
	float ContinentNoiseScale = 0.003f;

	// Decides the frequency of Mountains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0001", ClampMax = "0.03"))
	float TerrainNoiseScale = 0.01f;

	// Decides the frequency of Temperature Change
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0001", ClampMax = "0.01"))
	float TemperatureNoiseScale = 0.002f;
};

// --- Advanced Noise (노이즈 세부 조정) ---
USTRUCT(BlueprintType)
struct FOCGAdvancedNoiseSettings
{
	GENERATED_BODY()

	// Decides the difference between different noises (larger value gives more randomness)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0", ClampMax = "10000.0"))
	float StandardNoiseOffset = 10000.0f;

	// Decides how much the noise is spread out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "1.0", ClampMax = "10.0"))
	float RedistributionFactor = 2.5f;

	// Larger Octaves gives more detail to the landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "1", ClampMax = "10"))
	int32 Octaves = 3;

	// Larger Lacunarity gives more tight detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "1.0", ClampMax = "5.0"))
	float Lacunarity = 2.0f;

	// Larger Persistence gives more height change detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Persistence = 0.5f;
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

// --- River Settings (강 생성, Experimental) ---
USTRUCT(BlueprintType)
struct FOCGRiverSettings
{
	GENERATED_BODY()

	// Generates River. EXPERIMENTAL: has known issues; see team documentation before enabling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (DisplayName = "Generate River (Experimental)"))
	bool bGenerateRiver = false;

	// Seed for the River
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	int32 RiverSeed = 0;

	// Count of rivers to generate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10"))
	int32 RiverCount = 1;

	// Determines river's start point. 1.0 means the river will start at the highest point, 0.5 means middle height.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.5", ClampMax = "1.0", UIMin = "0.5", UIMax = "1.0"))
	float RiverSourceElevationRatio = 0.8f;

	// Intensity of simplifying river path. Higher value means more simplification.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "100", ClampMax = "1000", UIMin = "100", UIMax = "1000"))
	float RiverSplineSimplifyEpsilon = 200.0f;

	// Base of the river width.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	float RiverWidthBaseValue = 2048.0f;

	// Base of the river depth.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	float RiverDepthBaseValue = 1024.0f;

	// Base of the river velocity.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	float RiverVelocityBaseValue = 100.0f;

	// Minimum width of the river.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.0"))
	float RiverWidthMin = 50.0f;

	// Minimum depth of the river.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.0"))
	float RiverDepthMin = 20.0f;

	// Minimum velocity of the river.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.0"))
	float RiverVelocityMin = 5.0f;

	// Curve that defines the river's width based on distance from start point.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TObjectPtr<UCurveFloat> RiverWidthCurve = nullptr;

	// Curve that defines the river's depth based on distance from start point.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TObjectPtr<UCurveFloat> RiverDepthCurve = nullptr;

	// Curve that defines the river's velocity based on distance from start point.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TObjectPtr<UCurveFloat> RiverVelocityCurve = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> RiverWaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> RiverWaterStaticMeshMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> RiverToLakeTransitionMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TSoftObjectPtr<UMaterialInterface> RiverToOceanTransitionMaterial;
};

// --- Smoothing (스무딩 / 평활화) ---
USTRUCT(BlueprintType)
struct FOCGSmoothingSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoothing")
	bool bSmoothHeight = true;

	// Larger Radius gives softer smoothing effect
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothHeight", EditConditionHides, ClampMin = "5", ClampMax = "25")
	)
	int32 GaussianBlurRadius = 5;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothHeight", EditConditionHides)
	)
	bool bSmoothBySlope = false;

	// Larger Iteration takes more time but gives stronger smoothing
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothBySlope", EditConditionHides, ClampMin = "1", ClampMax = "5")
	)
	int32 SmoothingIteration = 3;

	// Slope larger than this angle will be smoothed
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothBySlope", EditConditionHides, ClampMin = "0.0", ClampMax = "89.9")
	)
	float MaxSlopeAngle = 60.0f;

	// Decides the strength of smoothing
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothBySlope", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0")
	)
	float SmoothingStrength = 0.5f;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothHeight", EditConditionHides)
	)
	bool bSmoothByMediumHeight = false;

	// Threshold Angle of the slope of the landscape
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Smoothing",
		meta = (EditCondition = "bSmoothByMediumHeight", EditConditionHides, ClampMin = "0", ClampMax = "5")
	)
	int32 MedianSmoothRadius = 3;
};

// --- Ocean (해양 워터 설정) ---
USTRUCT(BlueprintType)
struct FOCGOceanSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
	bool bContainWater = true;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> OceanWaterMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> OceanWaterStaticMeshMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> WaterHLODMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial;
};

// --- Landscape (지형 기본 설정) ---
USTRUCT(BlueprintType)
struct FOCGLandscapeSettings
{
	GENERATED_BODY()

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = 1, ClampMax = 16, UIMin = 1, UIMax = 16)
	)
	int32 WorldPartitionGridSize = 2;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64)
	)
	int32 WorldPartitionRegionSize = 16;

	// Horizontal size of your Landscape in Km (Changes Landscape Actor Scale)
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = 0.00001f)
	)
	float LandscapeSize = 1.009f;

	// Computed from LandscapeSize and MapResolution — not user-editable
	UPROPERTY()
	float LandscapeScale = 1;

	// If true changing LandscapeScale changes the terrain formation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	bool ApplyScaleToNoise = true;

	// Decides the grid spacing of debug landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (ClampMin = 1))
	int32 DebugGridSpacing = 16;

	// Decides the Blend radius(pixel) between different biomes
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = "0", ClampMax = "50")
	)
	int32 BiomeBlendRadius = 10;

	// Decides the Blend radius(pixel) between water and other biomes
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = "0", ClampMax = "50")
	)
	int32 WaterBlendRadius = 10;

	// The number of quads in a single landscape section.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	ELandscapeQuadsPerSection Landscape_QuadsPerSection = ELandscapeQuadsPerSection::Q63;

	// The number of sections in a single landscape component.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = "1", ClampMax = "2", UIMin = "1", UIMax = "2")
	)
	int32 Landscape_SectionsPerComponent = 1;

	// The number of components in the X and Y direction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	FIntPoint Landscape_ComponentCount = FIntPoint(16, 16);

	// The Resolution of landscape in X and Y direction
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Landscape",
		meta = (ClampMin = "63", ClampMax = "8129", UIMin = "63", UIMax = "8129")
	)
	FIntPoint MapResolution = FIntPoint(1009, 1009);

	// The Material used for Landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape")
	TObjectPtr<UMaterialInstance> LandscapeMaterial;

	// You can use your own Height Map Texture to generate landscape.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (FilePathFilter = "Image Files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|16-bit RAW (*.r16;*.raw)|*.r16;*.raw"))
	FFilePath HeightmapFilePath;
};
