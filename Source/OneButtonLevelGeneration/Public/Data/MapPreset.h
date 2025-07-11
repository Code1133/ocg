// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "OCGBiomeSettings.h"
#include "Structure/OCGHierarchyDataStructure.h"
#include "MapPreset.generated.h"

/**
 * 
 */

class AOCGLevelGenerator;
class AOCGLandscapeVolume;
class UPCGGraph;


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

	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void CalculateOptimalLooseness();
	void UpdateInternalMeshFilterNames();
	void UpdateInternalLandscapeFilterNames();

public:
	virtual UWorld* GetWorld() const override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	bool bContainWater = true;

	// If checked height, temperature, humidity, biome maps will be saved as PNG in Maps folder
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "OCG")
	bool bExportMaps = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	TArray<FOCGBiomeSettings> Biomes;

	FOCGBiomeSettings WaterBiome{TEXT("Water"), 0.f, 1.f, FLinearColor::Blue, 1, 0.5f};

public:
	/** The PCG graph to be used for generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	TObjectPtr<UPCGGraph> PCGGraph;

	/** Whether to show debug points for PCG generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	bool bShowDebugPoints = false;

	/** Whether to automatically generate the PCG graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	bool bAutoGenerate = true;

	/** Forces the regeneration of the PCG graph in the editor. */
	UFUNCTION(CallInEditor, Category = "PCG")
	void ForceGenerate() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	FRotator Landscape_Rotation;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	FVector Landscape_Location;
	
	UPROPERTY(EditAnywhere, Category = "World Settings | Basics | Landscape Settings")
	uint32 Landscape_RegionKilometer = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = 1, ClampMax = 16, UIMin = 1, UIMax = 16))
	int32 WorldPartitionGridSize = 2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64))
	int32 WorldPartitionRegionSize = 16;
	
	// Decides Landscape Size(Changes Landscape Actor Scale)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = 0.1f))
	float LandscapeScale = 1;

	// If true changing LandscapeScale changes the terrain formation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	bool ApplyScaleToNoise = true;
	
	// Decides the Blend radius(pixel) between different biomes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = "0", ClampMax = "50"))
	int32 BiomeBlendRadius = 10;

	// Decides the Blend radius(pixel) between water and other biomes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = "0", ClampMax = "50"))
	int32 WaterBlendRadius = 10;
	
	// Landscapes Minimum Height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height")
	float MinHeight = -15000.0f;

	// Landscapes Maximum Height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height")
	float MaxHeight = 20000.0f;

	// Decides the sea level height of landscape 0(Minimum height) ~ 1(Maximum height)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Height", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SeaLevel = 0.4f;

	// Landscapes Minimum Temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Temperature", meta = (ClampMin = -273.15))
	float MinTemp = -30.0f;

	// Landscapes Maximum Temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Temperature", meta = (ClampMin = -273.15))
	float MaxTemp = 80.0f;

	// Decides the amount of temperature drop per 1000 units of height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Temperature", meta = (ClampMin = "0.0"))
	float TempDropPer1000Units = 0.1f;

	// Decides the amount of humidity drop per distance from water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Humidity", meta = (ClampMin = "0.0"))
	float MoistureFalloffRate = 0.0005f;

	// Decides the amount of change in humidity caused by temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Humidity", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TemperatureInfluenceOnHumidity = 0.7f;

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	//bool bSmoothByMediumHeight = false;
	//
	//// Threshold Angle of the slope of the landscape
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
	//	meta = (EditCondition = "bSmoothByMediumHeight", EditConditionHides, ClampMin = "0", ClampMax = "5"))
	//int32 MedianSmoothRadius = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	bool bSmoothHeight = false;

	// Larger Radius gives softer smoothing effect
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
			meta = (EditCondition = "bSmoothHeight", EditConditionHides, ClampMin = "1", ClampMax = "25"))
	int32 GaussianBlurRadius = 5;
	
	//Island Properties
	// Decides whether the landscape will be island or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	bool bIsland = true;

	// Decides the sharpness of island edge and island's size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bIsland", EditConditionHides, ClampMin = 0.1, ClampMax = 3.0))
	float IslandFalloffExponent = 2.0f;

	// Decides irregularity of island shape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bIsland", EditConditionHides, ClampMin = "0.0001", ClampMax = "0.05"))
	float IslandShapeNoiseScale = 0.0025f;

	// Decides irregularity of island edge
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bIsland", EditConditionHides, ClampMin = 0.0001))
	float IslandShapeNoiseStrength = 0.5f;
	
	// Modify Terrain Properties
	// Decides whether the Mountain Ratio of biomes will be applied or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height")
	bool bModifyTerrainByBiome = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = 0.0f, ClampMax = 1.0f))
	float PlainSmoothFactor = 1.f;
	
	// Decides the frequency of details in Biome
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = "0.0001", ClampMax = "0.05"))
	float BiomeNoiseScale = 0.01f;

	// Decides the height of details in Biome
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
		meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = "0.0001", ClampMax = "1.0"))
	float BiomeNoiseAmplitude = 0.2f;

	// Larger radius gives smaller spike height difference at biome borders
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Height",
			meta = (EditCondition = "bModifyTerrainByBiome", EditConditionHides, ClampMin = "0", ClampMax = "50"))
	int32 BiomeHeightBlendRadius = 5;
	
public:
	// --- Noise Settings ---
	// Decides the difference between different noises (larger value gives more randomness)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "0.0"))
	float StandardNoiseOffset = 10000.f;

	// Decides how much the noise is spread out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "1.0"))
	float RedistributionFactor = 2.5f;
	
	// Decides the frequency of Mountains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise", meta = (ClampMin = "0.0001", ClampMax = "0.005"))
	float ContinentNoiseScale = 0.003f;

	// Decides the frequency of Mountains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise", meta = (ClampMin = "0.0001", ClampMax = "0.03"))
	float TerrainNoiseScale = 0.01f;

	// Decides the frequency of Temperature Change
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Noise", meta = (ClampMin = "0.0001", ClampMax = "0.01"))
	float TemperatureNoiseScale = 0.002f;

	// Larger Octaves gives more detail to the landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "1"))
	int32 Octaves = 3; // 노이즈 겹치는 횟수 (많을수록 디테일 증가)

	// Larger Lancunarity gives more tight detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "1.0"))
	float Lacunarity = 2.0f; // 주파수 변화율 (클수록 더 작고 촘촘한 노이즈 추가)

	// Larger Persistence give more height change detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Noise", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Persistence = 0.5f; // 진폭 변화율 (작을수록 추가되는 노이즈의 높이가 낮아짐)

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion")
	bool bErosion = true;

	// More Iteration gives more erosion details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "1"))
	int32 NumErosionIterations = 100000;

	// Decides the size of erosion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "3"))
	int32 ErosionRadius = 3;

	// Larger Inertia gives more smooth flow of erosion droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "0.99"))
	float DropletInertia = 0.25f; // 1에 가까울 수록 직진 성향 강해짐 0에 가까울수록 기울기에 따른 무작위 움직임

	// Decides the capacity of sediment one droplet can have
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0"))
	float SedimentCapacityFactor = 10.0f; // 흙 운반 용량 계수

	// Decides the minimum capacity of sediment one droplet can have
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0"))
	float MinSedimentCapacity = 0.01f; // 최소 운반 용량

	// Decides the speed of erosion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0"))
	float ErodeSpeed = 0.3f; // 침식 속도

	// Decides the speed of deposit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0"))
	float DepositSpeed = 0.3f; // 퇴적 속도

	// Decides how fast the droplet evaporates
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0", ClampMax = "1.0"))
	float EvaporateSpeed = 0.01f; // 증발 속도

	// Decides the gravity effect on droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0"))
	float Gravity = 9.8f;

	// Decides the maximum lifetime of droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0"))
	int32 MaxDropletLifetime = 50;

	// Decides the initial water volume of droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0"))
	float InitialWaterVolume = 0.5f;

	// Decides the initial speed of droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Advanced | Erosion",
		meta = (EditCondition = "bErosion", EditConditionHides, ClampMin = "0.0"))
	float InitialSpeed = 2.0f;

public:
	// The number of quads in a single landscape section. One section is the unit of LOD transition for landscape rendering.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "World Settings | Basics | Landscape Settings")
	ELandscapeQuadsPerSection Landscape_QuadsPerSection = ELandscapeQuadsPerSection::Q63;

	// The number of sections in a single landscape component. This along with the section size determines the size of each landscape component. A component is the base unit of rendering and culling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "World Settings | Basics | Landscape Settings", meta=(ClampMin="1", ClampMax="2", UIMin="1", UIMax="2"))
	int32 Landscape_SectionsPerComponent = 1;

	// The number of components in the X and Y direction, determining the overall size of the landscape.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	FIntPoint Landscape_ComponentCount = FIntPoint(16, 16);

	// The Resolution of landscape, including resolution of different maps used for landscape generation, in X and Y direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings", meta = (ClampMin = "63", ClampMax = "8129", UIMin = "63", UIMax = "8129"))
	FIntPoint MapResolution = FIntPoint(1009, 1009);

public:
	// Generates River.
	UFUNCTION(CallInEditor, Category = "World Settings")
	void RegenerateRiver();

	// Generates River. If true, the following river settings will be displayed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings")
	uint8 bGenerateRiver : 1 = false;

	// Count of rivers to generate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0", ClampMax = "10", UIMin = "0", UIMax = "10"))
	int32 RiverCount = 1;

	// Determines river's start point. 1.0 means the river will start at the highest point of the landscape, 0.5 means it will start at the middle height.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "0.5", ClampMax = "1.0", UIMin = "0.5", UIMax = "1.0"))
	float RiverStartPointThresholdMultiplier = 0.8f;

	// Intensity of Simplifing River Path. Higher value means more simplification, lower value means less simplification.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides, ClampMin = "100", ClampMax = "1000", UIMin = "100", UIMax = "1000"))
	float RiverSpineSimplifyEpsilon = 200.f;
	
	// Base of the river width. RiverWidthCurve value will be normalized and multiplied by this value to get the final width of the river.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	float RiverWidthBaseValue = 2048.0f;

	// Base of the river depth. RiverDepthCurve value will be normalized and multiplied by this value to get the final depth of the river.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	float RiverDepthBaseValue = 1024.0f;

	// Base of the river velocity. RiverVelocityCurve value will be normalized and multiplied by this value to get the final velocity of the river.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	float RiverVelocityBaseValue = 100.0f;
	
	// Curve that defines the river's width based on its distance from the start point. The X-axis represents the distance along the river, and the Y-axis represents the width.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | River Settings", meta = (EditCondition = "bGenerateRiver", EditConditionHides))
	TObjectPtr<UCurveFloat> RiverWidthCurve;

public:
	// The Material used for Landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics | Landscape Settings")
	TObjectPtr<UMaterialInstance> LandscapeMaterial;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hierarchy Data Settings")
	TArray<FLandscapeHierarchyData> HierarchiesData;

#if WITH_EDITOR
public:
	UPROPERTY(Transient)
	TWeakObjectPtr<AOCGLevelGenerator> LandscapeGenerator = nullptr;

	TWeakPtr<class FMapPresetEditorToolkit> EditorToolkit;
#endif
};
