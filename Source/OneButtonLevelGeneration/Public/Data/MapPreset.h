// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OCGBiomeSettings.h"
#include "Structure/OCGHierarchyDataStructure.h"
#include "MapPreset.generated.h"

/**
 * 
 */

class UOCGHierarchyDataAsset;
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
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	bool bContainWater = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	TArray<FOCGBiomeSettings> Biomes;

	FOCGBiomeSettings WaterBiome{TEXT("Water"), 0.f, 1.f, FLinearColor::Blue, 1, 0.5f};

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	TObjectPtr<UPCGGraph> PCGGraph;

public:
	//Decides Landscape Size(Changes Landscape Actor Scale)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (ClampMin = 0.1f))
	float LandscapeScale = 1;

	//If true changing LandscapeScale changes the terrain formation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (ClampMin = 0.f))
	bool ApplyScaleToNoise = true;
	
	//Decides the Blend radius(pixel) between different biomes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (ClampMin = "0", AllowPrivateAccess="true"))
	int32 BiomeBlendRadius = 10;

	//Decides the Blend radius(pixel) between water and other biomes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (ClampMin = "0", AllowPrivateAccess="true"))
	int32 WaterBlendRadius = 10;
	
	//Landscapes Minimum Height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = -32768.f, AllowPrivateAccess="true"))
	float MinHeight = -15000.0f;

	//Landscapes Maximum Height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMax = 32768.f, AllowPrivateAccess="true"))
	float MaxHeight = 20000.0f;

	//Decides the sea level height of landscape 0(Minimum height) ~ 1(Maximum height)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true") )
	float SeaLevel = 0.4f;

	//Decides whether the landscape will be island or not
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height")
	bool bIsland = true;

	//Decides the sharpness of island edge and island's size
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = 0.1))
	float IslandFaloffExponent = 2.0f;

	//Decides irregularity of island shape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = 0.0))
	float IslandShapeNoiseScale = 0.0025f;

	//Decides irregularity of island edge
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = 0.0))
	float IslandShapeNoiseStrength = 0.5f;

	//Landscapes Minimum Temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Temperature", meta = (AllowPrivateAccess="true"))
	float MinTemp = -30.0f;

	//Landscapes Maximum Temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Temperature", meta = (AllowPrivateAccess="true"))
	float MaxTemp = 80.0f;

	//Decides the amount of temperature drop per 1000 units of height
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Temperature", meta = (AllowPrivateAccess="true"))
	float TempDropPer1000Units = 0.1f;

	//Decides the amount of humidity drop per distance from water
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Humidity", meta = (ClampMin = "0.0", AllowPrivateAccess="true"))
	float MoistureFalloffRate = 0.0005f;

	//Decides the amount of change in humidity caused by temperature
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Humidity", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true"))
	float TemperatureInfluenceOnHumidity = 0.7f;

public:
	// --- Noise Settings ---
	//Decides the difference between different noises (larger value gives more randomness)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.0",AllowPrivateAccess="true"))
	float StandardNoiseOffset = 10000.f;

	//Decides how much the noise is spread out
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.0",AllowPrivateAccess="true"))
	float RedistributionFactor = 2.5f;
	
	//Decides the frequency of Mountains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.0",AllowPrivateAccess="true")) 
	float ContinentNoiseScale = 0.003f;

	//Decides the frequency of Mountains
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.0", AllowPrivateAccess="true"))
	float TerrainNoiseScale = 0.01f;

	//Larger Octaves gives more detail to the landscape
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.0",AllowPrivateAccess="true"))
	int32 Octaves = 3; // 노이즈 겹치는 횟수 (많을수록 디테일 증가)

	//Larger Lancunarity gives more tight detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.01", AllowPrivateAccess="true"))
	float Lacunarity = 2.0f; // 주파수 변화율 (클수록 더 작고 촘촘한 노이즈 추가)

	//Larger Persistence give more height change detail
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Noise", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true"))
	float Persistence = 0.5f; // 진폭 변화율 (작을수록 추가되는 노이즈의 높이가 낮아짐)

public:
	//More Iteration gives more erosion details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	int32 NumErosionIterations = 100000;

	//Decides the size of erosion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "1"))
	int32 ErosionRadius = 2;

	//Larger Inertia gives more smooth flow of erosion droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float DropletInertia = 0.25f; //1에 가까울 수록 직진 성향 강해짐 0에 가까울수록 기울기에 따른 무작위 움직임

	//Decides the capacity of sediment one droplet can have
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float SedimentCapacityFactor = 10.0f; // 흙 운반 용량 계수

	//Decides the minimum capacity of sediment one droplet can have
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float MinSedimentCapacity = 0.01f; // 최소 운반 용량

	//Decides the speed of erosion
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float ErodeSpeed = 0.3f; // 침식 속도

	//Decides the speed of deposit
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float DepositSpeed = 0.3f; // 퇴적 속도

	//Decides how fast the droplet evaporates
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float EvaporateSpeed = 0.01f; // 증발 속도

	//Decides the gravity effect on droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float Gravity = 9.8f;

	//Decides the maximum lifetime of droplets 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	int32 MaxDropletLifetime = 50;

	//Decides the initial water volume of droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float InitialWaterVolume = 0.5f;

	//Decides the initial speed of droplets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Erosion", meta = (ClampMin = "0.0"))
	float InitialSpeed = 2.0f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape Settings", meta=(AllowPrivateAccess="true"))
	ELandscapeQuadsPerSection Landscape_QuadsPerSection = ELandscapeQuadsPerSection::Q63;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape Settings", meta=(AllowPrivateAccess="true", ClampMin="1", ClampMax="2", UIMin="1", UIMax="2"))
	int32 Landscape_SectionsPerComponent = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Settings")
	FIntPoint Landscape_ComponentCount = FIntPoint(16, 16);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Settings", meta = (ClampMin = "63", ClampMax = "8129", UIMin = "63", UIMax = "8129", AllowPrivateAccess="true"))
	FIntPoint MapResolution = FIntPoint(1009, 1009);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Settings")
	TObjectPtr<UMaterialInstance> LandscapeMaterial;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hierarchy Data Settings")
	TArray<FLandscapeHierarchyData> HierarchiesData;

#if WITH_EDITOR
public:
	UPROPERTY()
	UWorld* OwnerWorld = nullptr;

	TWeakPtr<class FMapPresetEditorToolkit> EditorToolkit;
#endif
};
