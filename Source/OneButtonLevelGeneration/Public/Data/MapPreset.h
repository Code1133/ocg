// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OCGBiomeSettings.h" 
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
	bool bUseOwnMaterMaterial = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	TArray<FOCGBiomeSettings> Biomes;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	TObjectPtr<UOCGHierarchyDataAsset> PCGHierarchyData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG")
	TObjectPtr<UPCGGraph> PCGGraph;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (ClampMin = 0.f))
	float LandscapeScale = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (ClampMin = 0.f))
	bool ApplyScaleToNoise = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = -32768.f, AllowPrivateAccess="true"))
	float MinHeight = -15000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMax = 32768.f, AllowPrivateAccess="true"))
	float MaxHeight = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true") )
	float SeaLevel = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height")
	bool bIsland = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height")
	float IslandFaloffExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height")
	float IslandShapeNoiseScale = 0.0025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Height")
	float IslandShapeNoiseStrength = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Temperature", meta = (AllowPrivateAccess="true"))
	float MinTemp = -30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Temperature", meta = (AllowPrivateAccess="true"))
	float MaxTemp = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Temperature", meta = (AllowPrivateAccess="true"))
	float TempDropPer1000Units = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Humidity", meta = (ClampMin = "0.0", AllowPrivateAccess="true"))
	float MoistureFalloffRate = 0.0005f;

	// 온도가 습도에 미치는 영향 (클수록 더운 지역이 더 건조해짐)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Humidity", meta = (ClampMin = "0.0", AllowPrivateAccess="true"))
	float TemperatureInfluenceOnHumidity = 0.7f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings | Basics", meta = (AllowPrivateAccess="true"))
	int32 BiomeBlendRadius = 10;

public:
	// --- Noise Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (AllowPrivateAccess="true"))
	float StandardNoiseOffset = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (AllowPrivateAccess="true"))
	float RedistributionFactor = 2.5f;

	// 대륙의 큰 형태를 만드는 저주파 노이즈 스케일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (AllowPrivateAccess="true")) 
	float ContinentNoiseScale = 0.003f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (DisplayName = "Terrain Noise Scale", AllowPrivateAccess="true"))
	float HeightNoiseScale = 0.01f;

	// 대륙 노이즈가 지형 노이즈에 미치는 영향력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true"))
	float ContinentInfluence = 0.7f;
 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (AllowPrivateAccess="true"))
	int32 Octaves = 3; // 노이즈 겹치는 횟수 (많을수록 디테일 증가)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (ClampMin = "0.01", AllowPrivateAccess="true"))
	float Lacunarity = 2.0f; // 주파수 변화율 (클수록 더 작고 촘촘한 노이즈 추가)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true"))
	float Persistence = 0.5f; // 진폭 변화율 (작을수록 추가되는 노이즈의 높이가 낮아짐)

public:
	//Erosion Settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	int32 NumErosionIterations = 100000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion", meta = (ClampMin = "2"))
	int32 ErosionRadius = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float DropletInertia = 0.25f; //1에 가까울 수록 직진 성향 강해짐 0에 가까울수록 기울기에 따른 무작위 움직임

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float SedimentCapacityFactor = 10.0f; // 흙 운반 용량 계수

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float MinSedimentCapacity = 0.01f; // 최소 운반 용량

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float ErodeSpeed = 0.3f; // 침식 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float DepositSpeed = 0.3f; // 퇴적 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float EvaporateSpeed = 0.01f; // 증발 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float Gravity = 9.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	int32 MaxDropletLifetime = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float InitialWaterVolume = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Erosion")
	float InitialSpeed = 2.0f;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape Settings", meta=(AllowPrivateAccess="true"))
	ELandscapeQuadsPerSection Landscape_QuadsPerSection = ELandscapeQuadsPerSection::Q63;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape Settings", meta=(AllowPrivateAccess="true", ClampMin="1", ClampMax="2", UIMin="1", UIMax="2"))
	int32 Landscape_SectionsPerComponent = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Settings")
	FIntPoint Landscape_ComponentCount = FIntPoint(8, 8);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Settings", meta = (ClampMin = "63", ClampMax = "8129", UIMin = "63", UIMax = "8129", AllowPrivateAccess="true"))
	FIntPoint MapResolution = FIntPoint(1009, 1009);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape Settings")
	TObjectPtr<UMaterialInstance> LandscapeMaterial;

#if WITH_EDITOR
public:
	UPROPERTY()
	UWorld* OwnerWorld = nullptr;

	class FMapPresetEditorToolkit* EditorToolkit = nullptr;
#endif
};
