// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "OCGDeveloperSettings.generated.h"

class UPCGGraph;
class URuntimeVirtualTexture;


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
