// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGLandscapeGenerateComponent.generated.h"

class UMapPreset;
class ALandscapeProxy;
class ULandscapeLayerInfoObject;
class AOCGLevelGenerator;
struct FOCGBiomeSettings;
struct FLandscapeImportLayerInfo;
class ALandscape;
class URuntimeVirtualTexture;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONEBUTTONLEVELGENERATION_API UOCGLandscapeGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOCGLandscapeGenerateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
public:
	ALandscape* GetLandscape() const { return TargetLandscape; }
	const TArray<FIntPoint>& GetCachedRiverStartPoints() const { return CachedRiverStartPoints; }
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	TObjectPtr<ALandscape> TargetLandscape;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionGridSize = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionRegionSize = 16;
public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscapeInEditor();
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscape(UWorld* World);
	
private:
	void FinalizeLayerInfos(ALandscape* Landscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> PrepareLandscapeLayerData(ALandscape* InTargetLandscape, AOCGLevelGenerator* InLevelGenerator, const UMapPreset* InMapPreset);

	ULandscapeLayerInfoObject* CreateLayerInfo(ALandscape* InLandscape, const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

	ULandscapeLayerInfoObject* CreateLayerInfo(const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

	FString LayerInfoSavePath = TEXT("/Game/Landscape/LayerInfos");
	
	AOCGLevelGenerator* GetLevelGenerator() const;

	bool CreateRuntimeVirtualTextureVolume(ALandscape* InLandscapeActor);

	FVector GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const;
	void CachePointsForRiverGeneration();

	TObjectPtr<URuntimeVirtualTexture> ColorRVT = nullptr;
	TObjectPtr<URuntimeVirtualTexture> HeightRVT = nullptr;
	TObjectPtr<URuntimeVirtualTexture> DisplacementRVT = nullptr;
private:
	float CachedGlobalMinTemp;
	float CachedGlobalMaxTemp;
	float CachedGlobalMinHumidity;
	float CachedGlobalMaxHumidity;

	TArray<FIntPoint> CachedRiverStartPoints;
};
