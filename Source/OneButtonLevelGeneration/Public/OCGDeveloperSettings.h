// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "OCGDeveloperSettings.generated.h"

class UPCGGraph;
class URuntimeVirtualTexture;

class UOCGHeightmapStrategyBase;
class UOCGTemperatureStrategyBase;
class UOCGHumidityStrategyBase;
class UOCGBiomeStrategyBase;
class UOCGTerrainModifierStrategyBase;
class UOCGErosionStrategyBase;
class UOCGSmoothingStrategyBase;


/**
 * 플러그인이 사용하는 에셋 경로의 기본 경로 모음입니다.
 */
UCLASS(config = OneButtonLevelGeneration, DefaultConfig, meta = (DisplayName = "One Button Level Generation Settings"))
class ONEBUTTONLEVELGENERATION_API UOCGDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Default material for landscape generation */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Asset References")
	TSoftObjectPtr<UMaterialInstance> DefaultLandscapeMaterialPath;

	/** Default PCG Graph for level generation */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Asset References")
	TSoftObjectPtr<UPCGGraph> DefaultPCGGraphPath;

	// --- Runtime Virtual Texture ---

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Runtime Virtual Texture")
	TSoftObjectPtr<URuntimeVirtualTexture> DefaultColorRVT;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Runtime Virtual Texture")
	TSoftObjectPtr<URuntimeVirtualTexture> DefaultHeightRVT;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Runtime Virtual Texture")
	TSoftObjectPtr<URuntimeVirtualTexture> DefaultDisplacementRVT;

	// --- Output Paths ---

	/** 생성된 LandscapeLayerInfo 에셋이 저장될 경로 */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Output Paths", meta = (ContentDir))
	FDirectoryPath LayerInfoSavePath;

	// --- Generation Strategies ---
	//
	// 생성 파이프라인의 각 단계를 교체하기 위한 확장점입니다.
	// 대응하는 Base 클래스를 C++로 상속한 뒤 여기서 지정하면 기본 구현 대신 그 클래스가 쓰입니다.
	// 비워 두거나 추상 클래스를 넣으면 기본 구현으로 폴백합니다.
	//
	// Blueprint 상속은 지원하지 않습니다. 전략이 주고받는 FOCGWorldDataContainer가
	// USTRUCT가 아니고, TArray<uint16>·TMap<FName, TArray<uint8>>처럼
	// Blueprint가 표현할 수 없는 타입을 담고 있기 때문입니다.

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGHeightmapStrategyBase> HeightmapStrategyClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGTemperatureStrategyBase> TemperatureStrategyClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGHumidityStrategyBase> HumidityStrategyClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGBiomeStrategyBase> BiomeStrategyClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGTerrainModifierStrategyBase> TerrainModifierStrategyClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGErosionStrategyBase> ErosionStrategyClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Generation Strategies")
	TSubclassOf<UOCGSmoothingStrategyBase> SmoothingStrategyClass;

	/**
	 * 설정된 에셋 경로 중 실제로 존재하지 않는 항목을 반환합니다.
	 * 에셋을 로드하지 않고 AssetRegistry로 존재만 조회합니다.
	 *
	 * @return "프로퍼티이름 = 경로" 형식의 실패 목록. 모두 정상이면 비어 있습니다.
	 */
	TArray<FString> FindUnresolvedAssets() const;

	/**
	 * FindUnresolvedAssets 결과를 Error 로그로 남깁니다.
	 *
	 * @note ini는 Fix Up Redirectors 시 자동 갱신되지 않으므로 이 검증이 필요합니다.
	 */
	void ValidateConfiguredAssets() const;
};
