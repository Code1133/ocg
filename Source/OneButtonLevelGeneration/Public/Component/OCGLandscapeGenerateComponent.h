// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FileHelpers.h"
#include "FileHelpers.h"
#include "Components/ActorComponent.h"
#include "OCGLandscapeGenerateComponent.generated.h"

class ARuntimeVirtualTextureVolume;
class UMapPreset;
class ALandscapeProxy;
class ULandscapeLayerInfoObject;
class AOCGLevelGenerator;
struct FOCGBiomeSettings;
struct FLandscapeImportLayerInfo;
class ALandscape;
class URuntimeVirtualTexture;

class ALocationVolume;
class ULandscapeSubsystem;
class ULandscapeInfo;

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
	void GetLandscapeZScale(float ZScale) {LandscapeZScale = ZScale;}
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	TObjectPtr<ALandscape> TargetLandscape;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionGridSize = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionRegionSize = 16;

	int32 QuadsPerSection;
	FIntPoint TotalLandscapeComponentSize;
	int32 ComponentCountX;
	int32 ComponentCountY;
	int32 QuadsPerComponent;
	int32 SizeX;
	int32 SizeY;
public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscapeInEditor();
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscape(UWorld* World);
	
private:
	void OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bPromptUser);
	
	void InitializeLandscapeSetting(UWorld* World);
	
	void AddTargetLayers(ALandscape* Landscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	void ManageLandscapeRegions(UWorld* World, ALandscape* Landscape);

	void AddLandscapeComponent(ULandscapeInfo* InLandscapeInfo, ULandscapeSubsystem* InLandscapeSubsystem, const TArray<FIntPoint>& InComponentCoordinates, TArray<ALandscapeProxy*>& OutCreatedStreamingProxies);

	ALocationVolume* CreateLandscapeRegionVolume(UWorld* InWorld, ALandscapeProxy* InParentLandscapeActor, const FIntPoint& InRegionCoordinate, double InRegionSize);

	void ForEachComponentByRegion(int32 RegionSize, const TArray<FIntPoint>& ComponentCoordinates, TFunctionRef<bool(const FIntPoint&, const TArray<FIntPoint>&)> RegionFn);

	template<typename T>
	void SaveObjects(TArrayView<T*> InObjects)
	{
		TArray<UPackage*> Packages;
		Algo::Transform(InObjects, Packages, [](UObject* InObject) { return InObject->GetPackage(); });
		UEditorLoadingAndSavingUtils::SavePackages(Packages, /* bOnlyDirty = */ false);
	}
	
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
	float LandscapeZScale;

	TArray<FIntPoint> CachedRiverStartPoints;

	TArray<ARuntimeVirtualTextureVolume*> CachedRuntimeVirtualTextureVolumes;

};
