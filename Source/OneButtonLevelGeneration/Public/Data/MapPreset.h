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

UCLASS(BlueprintType, meta = (DisplayName = "Map Preset"))
class ONEBUTTONLEVELGENERATION_API UMapPreset : public UObject
{
	GENERATED_BODY()

public:
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	FOCGLandscapeSettings LandscapeSettings;

#pragma region Deprecated Landscape Settings
	UPROPERTY() int32 WorldPartitionGridSize_DEPRECATED = 2;
	UPROPERTY() int32 WorldPartitionRegionSize_DEPRECATED = 16;
	UPROPERTY() float LandscapeSize_DEPRECATED = 1.009f;
	UPROPERTY() float LandscapeScale_DEPRECATED = 1;
	UPROPERTY() bool ApplyScaleToNoise_DEPRECATED = true;
	UPROPERTY() int32 DebugGridSpacing_DEPRECATED = 16;
	UPROPERTY() int32 BiomeBlendRadius_DEPRECATED = 10;
	UPROPERTY() int32 WaterBlendRadius_DEPRECATED = 10;
	UPROPERTY() ELandscapeQuadsPerSection Landscape_QuadsPerSection_DEPRECATED = ELandscapeQuadsPerSection::Q63;
	UPROPERTY() int32 Landscape_SectionsPerComponent_DEPRECATED = 1;
	UPROPERTY() FIntPoint Landscape_ComponentCount_DEPRECATED = FIntPoint(16, 16);
	UPROPERTY() FIntPoint MapResolution_DEPRECATED = FIntPoint(1009, 1009);
	UPROPERTY() TObjectPtr<UMaterialInstance> LandscapeMaterial_DEPRECATED;
	UPROPERTY() FFilePath HeightmapFilePath_DEPRECATED;
#pragma endregion

	// --- Height ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height")
	FOCGHeightSettings HeightSettings;

#pragma region Deprecated Height Settings
	UPROPERTY() float MinHeight_DEPRECATED = -15000.0f;
	UPROPERTY() float MaxHeight_DEPRECATED = 20000.0f;
	UPROPERTY() float SeaLevel_DEPRECATED = 0.4f;
#pragma endregion

	// --- Temperature ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Temperature")
	FOCGTemperatureSettings TemperatureSettings;

#pragma region Deprecated Temperature Settings
	UPROPERTY() float MinTemp_DEPRECATED = -30.0f;
	UPROPERTY() float MaxTemp_DEPRECATED = 80.0f;
#pragma endregion

	// --- Noise ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise")
	FOCGBasicNoiseSettings BasicNoiseSettings;

#pragma region Deprecated Basic Noise Settings
	UPROPERTY() float ContinentNoiseScale_DEPRECATED = 0.003f;
	UPROPERTY() float TerrainNoiseScale_DEPRECATED = 0.01f;
	UPROPERTY() float TemperatureNoiseScale_DEPRECATED = 0.002f;
#pragma endregion

	// ============================== World Settings : Advanced ==============================

	// --- Height (Smoothing / Island / Biome Terrain) ---
	// Smoothing Properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	FOCGSmoothingSettings SmoothingSettings;

#pragma region Deprecated Smoothing Settings
	UPROPERTY() bool bSmoothHeight_DEPRECATED = true;
	UPROPERTY() int32 GaussianBlurRadius_DEPRECATED = 5;
	UPROPERTY() bool bSmoothBySlope_DEPRECATED = false;
	UPROPERTY() int32 SmoothingIteration_DEPRECATED = 3;
	UPROPERTY() float MaxSlopeAngle_DEPRECATED = 60.0f;
	UPROPERTY() float SmoothingStrength_DEPRECATED = 0.5f;
	UPROPERTY() bool bSmoothByMediumHeight_DEPRECATED = false;
	UPROPERTY() int32 MedianSmoothRadius_DEPRECATED = 3;
#pragma endregion

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise")
	FOCGAdvancedNoiseSettings AdvancedNoiseSettings;

#pragma region Deprecated Advanced Noise Settings
	UPROPERTY() float StandardNoiseOffset_DEPRECATED = 10000.0f;
	UPROPERTY() float RedistributionFactor_DEPRECATED = 2.5f;
	UPROPERTY() int32 Octaves_DEPRECATED = 3;
	UPROPERTY() float Lacunarity_DEPRECATED = 2.0f;
	UPROPERTY() float Persistence_DEPRECATED = 0.5f;
#pragma endregion

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
	FOCGOceanSettings OceanSettings;

#pragma region Deprecated Ocean Settings
	UPROPERTY() bool bContainWater_DEPRECATED = true;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> OceanWaterMaterial_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> OceanWaterStaticMeshMaterial_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> WaterHLODMaterial_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> UnderwaterPostProcessMaterial_DEPRECATED;
#pragma endregion

public:
	// ============================== River Settings (Experimental) ==============================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "River Settings (Experimental)")
	FOCGRiverSettings RiverSettings;

#pragma region Deprecated River Settings
	UPROPERTY() bool bGenerateRiver_DEPRECATED = false;
	UPROPERTY() int32 RiverSeed_DEPRECATED = 0;
	UPROPERTY() int32 RiverCount_DEPRECATED = 1;
	UPROPERTY() float RiverSourceElevationRatio_DEPRECATED = 0.8f;
	UPROPERTY() float RiverSplineSimplifyEpsilon_DEPRECATED = 200.0f;
	UPROPERTY() float RiverWidthBaseValue_DEPRECATED = 2048.0f;
	UPROPERTY() float RiverDepthBaseValue_DEPRECATED = 1024.0f;
	UPROPERTY() float RiverVelocityBaseValue_DEPRECATED = 100.0f;
	UPROPERTY() float RiverWidthMin_DEPRECATED = 50.0f;
	UPROPERTY() float RiverDepthMin_DEPRECATED = 20.0f;
	UPROPERTY() float RiverVelocityMin_DEPRECATED = 5.0f;
	UPROPERTY() TObjectPtr<UCurveFloat> RiverWidthCurve_DEPRECATED;
	UPROPERTY() TObjectPtr<UCurveFloat> RiverDepthCurve_DEPRECATED;
	UPROPERTY() TObjectPtr<UCurveFloat> RiverVelocityCurve_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> RiverWaterMaterial_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> RiverWaterStaticMeshMaterial_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> RiverToLakeTransitionMaterial_DEPRECATED;
	UPROPERTY() TSoftObjectPtr<UMaterialInterface> RiverToOceanTransitionMaterial_DEPRECATED;
#pragma endregion

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
