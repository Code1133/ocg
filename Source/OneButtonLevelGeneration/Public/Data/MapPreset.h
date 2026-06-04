// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OCGBiomeSettings.h"
#include "OCGMapPresetSettings.h"
#include "Structure/OCGHierarchyDataStructure.h"
#include "MapPreset.generated.h"

class UPCGGraph;

/**
 * UMapPreset 프로퍼티가 변경될 때 브로드캐스트됩니다.
 * DataAsset은 월드를 직접 알 수 없으므로, 월드 액터 업데이트는
 * 이 델리게이트를 구독한 OCGEditorSubsystem이 담당합니다.
 *
 * @param ChangedPreset 변경된 프리셋 인스턴스
 * @param PropertyName 변경된 멤버 프로퍼티 이름
 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMapPresetPropertyChanged, const class UMapPreset*, FName /*PropertyName*/);

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

UCLASS(BlueprintType, meta = (DisplayName = "Map Preset"))
class ONEBUTTONLEVELGENERATION_API UMapPreset : public UObject
{
	GENERATED_BODY()
public:
	UMapPreset();

	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	/**
	 * 프로퍼티 변경 시 브로드캐스트되는 정적 델리게이트.
	 * OCGEditorSubsystem이 Initialize/Deinitialize에서 구독 관리를 담당합니다.
	 */
	static FOnMapPresetPropertyChanged OnPropertyChanged;

private:
	void CalculateOptimalLooseness();
	void UpdateInternalMeshFilterNames();
	void UpdateInternalLandscapeFilterNames();

public:
	// ============================== World Settings : Basics ==============================

	// --- Landscape ---
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta = (ClampMin = 1, ClampMax = 16, UIMin = 1, UIMax = 16)
	)
	int32 WorldPartitionGridSize = 2;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64)
	)
	int32 WorldPartitionRegionSize = 16;

	// Horizontal size of your Landscape in Km (Changes Landscape Actor Scale)
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta = (ClampMin = 0.00001f)
	)
	float LandscapeSize = 1.009f;

	UPROPERTY()
	float LandscapeScale = 1;

	// If true changing LandscapeScale changes the terrain formation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	bool ApplyScaleToNoise = true;

	// Decides the grid spacing of debug landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = 1))
	int32 DebugGridSpacing = 16;

	// Decides the Blend radius(pixel) between different biomes
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta = (ClampMin = "0", ClampMax = "50")
	)
	int32 BiomeBlendRadius = 10;

	// Decides the Blend radius(pixel) between water and other biomes
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta = (ClampMin = "0", ClampMax = "50")
	)
	int32 WaterBlendRadius = 10;

	// The number of quads in a single landscape section. One section is the unit of LOD transition for landscape rendering.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	ELandscapeQuadsPerSection Landscape_QuadsPerSection = ELandscapeQuadsPerSection::Q63;

	// The number of sections in a single landscape component. This along with the section size determines the size of each landscape component. A component is the base unit of rendering and culling.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta=(ClampMin="1", ClampMax="2", UIMin="1", UIMax="2")
	)
	int32 Landscape_SectionsPerComponent = 1;

	// The number of components in the X and Y direction, determining the overall size of the landscape.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	FIntPoint Landscape_ComponentCount = FIntPoint(16, 16);

	// The Resolution of landscape, including resolution of different maps used for landscape generation, in X and Y direction
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings",
		meta = (ClampMin = "63", ClampMax = "8129", UIMin = "63", UIMax = "8129")
	)
	FIntPoint MapResolution = FIntPoint(1009, 1009);

	// The Material used for Landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	TObjectPtr<UMaterialInstance> LandscapeMaterial;

	// You can use your own Height Map Texture to generate landscape. Texture resolution must be equal to Map Resolution.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (FilePathFilter = "Image Files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|16-bit RAW (*.r16;*.raw)|*.r16;*.raw"))
	FFilePath HeightmapFilePath;

	// --- Height ---
	// Landscapes Minimum Height (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height")
	float MinHeight = -15000.0f;

	// Landscapes Maximum Height (in cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height")
	float MaxHeight = 20000.0f;

	// Decides the sea level height of landscape 0(Minimum height) ~ 1(Maximum height)
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height",
		meta = (ClampMin = "0.0", ClampMax = "1.0")
	)
	float SeaLevel = 0.4f;

	// --- Temperature ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Temperature")
	FOCGTemperatureSettings TemperatureSettings;

#pragma region Deprecated Temperature Settings
	UPROPERTY() float MinTemp_DEPRECATED = -30.0f;
	UPROPERTY() float MaxTemp_DEPRECATED = 80.0f;
#pragma endregion

	// --- Noise ---
	// Decides the frequency of Mountains
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise",
		meta = (ClampMin = "0.0001", ClampMax = "0.005")
	)
	float ContinentNoiseScale = 0.003f;

	// Decides the frequency of Mountains
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise",
		meta = (ClampMin = "0.0001", ClampMax = "0.03")
	)
	float TerrainNoiseScale = 0.01f;

	// Decides the frequency of Temperature Change
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise",
		meta = (ClampMin = "0.0001", ClampMax = "0.01")
	)
	float TemperatureNoiseScale = 0.002f;

	// ============================== World Settings : Advanced ==============================

	// --- Height (Smoothing / Island / Biome Terrain) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	bool bSmoothHeight = true;

	// Larger Radius gives softer smoothing effect
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothHeight", EditConditionHides, ClampMin = "5", ClampMax = "25")
	)
	int32 GaussianBlurRadius = 5;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothHeight", EditConditionHides)
	)
	bool bSmoothBySlope = false;

	// Larger Iteration takes more time but gives stronger smoothing
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothBySlope", EditConditionHides, ClampMin = "1", ClampMax = "5")
	)
	int32 SmoothingIteration = 3;

	// Slope larger than this angle will be smoothed
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothBySlope", EditConditionHides, ClampMin = "0.0", ClampMax = "89.9")
	)
	float MaxSlopeAngle = 60.0f;

	// Decides the strength of smoothing
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothBySlope", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0")
	)
	float SmoothingStrength = 0.5f;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothHeight", EditConditionHides)
	)
	bool bSmoothByMediumHeight = false;

	// Threshold Angle of the slope of the landscape
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bSmoothByMediumHeight", EditConditionHides, ClampMin = "0", ClampMax = "5")
	)
	int32 MedianSmoothRadius = 3;

	// Island Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	FOCGIslandSettings IslandSettings;

#pragma region Deprecated Island Settings
	UPROPERTY() bool bIsland_DEPRECATED = true;
	UPROPERTY() float IslandFalloffExponent_DEPRECATED = 2.0f;
	UPROPERTY() float IslandShapeNoiseScale_DEPRECATED = 0.0025f;
	UPROPERTY() float IslandShapeNoiseStrength_DEPRECATED = 0.5f;
#pragma endregion

	// Modify Terrain Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	FOCGBiomeTerrainSettings BiomeTerrainSettings;

#pragma region Deprecated Biome Terrain Settings
	UPROPERTY() bool bModifyTerrainByBiome_DEPRECATED = false;
	UPROPERTY() float PlainSmoothFactor_DEPRECATED = 1.0f;
	UPROPERTY() float BiomeNoiseScale_DEPRECATED = 0.01f;
	UPROPERTY() float BiomeNoiseAmplitude_DEPRECATED = 0.2f;
	UPROPERTY() int32 BiomeHeightBlendRadius_DEPRECATED = 5;
#pragma endregion

	// --- Temperature ---
	// Decides the amount of temperature drop per 1000 units of height
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Temperature",
		meta = (ClampMin = "0.0")
	)
	float TempDropPer1000Units = 0.1f;

	// --- Humidity ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Humidity")
	FOCGHumiditySettings HumiditySettings;

#pragma region Deprecated Humidity Settings
	UPROPERTY() float MoistureFalloffRate_DEPRECATED = 0.0005f;
	UPROPERTY() float TemperatureInfluenceOnHumidity_DEPRECATED = 0.7f;
#pragma endregion

	// --- Noise ---
	// Decides the difference between different noises (larger value gives more randomness)
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "0.0", ClampMax = "10000.0")
	)
	float StandardNoiseOffset = 10000.0f;

	// Decides how much the noise is spread out
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "1.0", ClampMax = "10.0")
	)
	float RedistributionFactor = 2.5f;

	// Larger Octaves gives more detail to the landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "1", ClampMax = "10"))
	int32 Octaves = 3; // 노이즈 겹치는 횟수 (많을수록 디테일 증가)

	// Larger Lancunarity gives more tight detail
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "1.0", ClampMax = "5.0")
	)
	float Lacunarity = 2.0f; // 주파수 변화율 (클수록 더 작고 촘촘한 노이즈 추가)

	// Larger Persistence give more height change detail
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise",
		meta = (ClampMin = "0.0", ClampMax = "1.0")
	)
	float Persistence = 0.5f; // 진폭 변화율 (작을수록 추가되는 노이즈의 높이가 낮아짐)

	// --- Erosion ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion")
	FOCGErosionSettings ErosionSettings;

#pragma region Deprecated Erosion Settings
	UPROPERTY() bool bErosion_DEPRECATED = true;
	UPROPERTY() int32 NumErosionIterations_DEPRECATED = 100000;
	UPROPERTY() int32 ErosionRadius_DEPRECATED = 3;
	UPROPERTY() float DropletInertia_DEPRECATED = 0.25f;
	UPROPERTY() float SedimentCapacityFactor_DEPRECATED = 10.0f;
	UPROPERTY() float MinSedimentCapacity_DEPRECATED = 0.01f;
	UPROPERTY() float ErodeSpeed_DEPRECATED = 0.3f;
	UPROPERTY() float DepositSpeed_DEPRECATED = 0.3f;
	UPROPERTY() float EvaporateSpeed_DEPRECATED = 0.01f;
	UPROPERTY() float Gravity_DEPRECATED = 9.8f;
	UPROPERTY() int32 MaxDropletLifetime_DEPRECATED = 50;
	UPROPERTY() float InitialWaterVolume_DEPRECATED = 0.5f;
	UPROPERTY() float InitialSpeed_DEPRECATED = 2.0f;
#pragma endregion

public:
	// ============================== Ocean Settings ==============================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean Settings")
	bool bContainWater = true;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean Settings",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> OceanWaterMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean Settings",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> OceanWaterStaticMeshMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean Settings",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> WaterHLODMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "Ocean Settings",
		meta = (EditCondition = "bContainWater", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial;

public:
	// ============================== River Settings (Experimental) ==============================
	// Generates River. EXPERIMENTAL: has known issues; see team documentation before enabling.
	// If true, the following river settings will be displayed.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (DisplayName = "Generate River (Experimental)")
	)
	bool bGenerateRiver = false;

	// Seed for the River
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	int32 RiverSeed = 0;

	// Count of rivers to generate.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "1", ClampMax = "10", UIMin = "1",
			UIMax = "10")
	)
	int32 RiverCount = 1;

	// Determines river's start point. 1.0 means the river will start at the highest point of the landscape, 0.5 means it will start at the middle height.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.5", ClampMax = "1.0", UIMin = "0.5",
			UIMax = "1.0")
	)
	float RiverSourceElevationRatio = 0.8f;

	// Intensity of Simplifing River Path. Higher value means more simplification, lower value means less simplification.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "100", ClampMax = "1000", UIMin = "100"
			, UIMax = "1000")
	)
	float RiverSplineSimplifyEpsilon = 200.0f;

	// Base of the river width. RiverWidthCurve value will be normalized and multiplied by this value to get the final width of the river.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	float RiverWidthBaseValue = 2048.0f;

	// Base of the river depth. RiverDepthCurve value will be normalized and multiplied by this value to get the final depth of the river.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	float RiverDepthBaseValue = 1024.0f;

	// --- Advanced River Settings ---

	// Base of the river velocity. RiverVelocityCurve value will be normalized and multiplied by this value to get the final velocity of the river.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	float RiverVelocityBaseValue = 100.0f;

	// Minimum width of the river. This value is added to the calculated width.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.0")
	)
	float RiverWidthMin = 50.0f;

	// Minimum depth of the river. This value is added to the calculated depth.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.0")
	)
	float RiverDepthMin = 20.0f;

	// Minimum velocity of the river. This value is added to the calculated velocity.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.0")
	)
	float RiverVelocityMin = 5.0f;

	// Curve that defines the river's width based on its distance from the start point. The X-axis represents the distance along the river, and the Y-axis represents the width.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TObjectPtr<UCurveFloat> RiverWidthCurve;

	// Curve that defines the river's depth based on its distance from the start point. The X-axis represents the distance along the river, and the Y-axis represents the depth.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TObjectPtr<UCurveFloat> RiverDepthCurve;

	// Curve that defines the river's velocity based on its distance from the start point. The X-axis represents the distance along the river, and the Y-axis represents the velocity.
	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TObjectPtr<UCurveFloat> RiverVelocityCurve;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> RiverWaterMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> RiverWaterStaticMeshMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> RiverToLakeTransitionMaterial;

	UPROPERTY(
		EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)",
		meta = (EditCondition = "bGenerateRiver", EditConditionHides)
	)
	TSoftObjectPtr<UMaterialInterface> RiverToOceanTransitionMaterial;

public:
	// ============================== PCG ==============================
	/** The PCG graph to be used for generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	TObjectPtr<UPCGGraph> PCGGraph;

	/** Whether to automatically generate the PCG graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	bool bAutoGenerate = true;

public:
	// ============================== OCG ==============================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	int32 Seed = 1337;

	// If checked height, temperature, humidity, biome maps will be saved as PNG in Maps folder
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	bool bExportMapTextures = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	TArray<FOCGBiomeSettings> Biomes;

	FOCGBiomeSettings WaterBiome{ TEXT("Water"), 0.0f, 1.0f, FLinearColor::Blue, 1, 0.5f };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OCG")
	TArray<FLandscapeHierarchyData> HierarchiesData;
};
