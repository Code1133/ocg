// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "FileHelpers.h"
#include "ActorPartition/ActorPartitionSubsystem.h"
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
	void GetLandscapeZValues(float ZScale, float ZOffset) {LandscapeZScale = ZScale; LandscapeZOffset =  ZOffset;}
	FVector GetVolumeExtent() const { return VolumeExtent; }
	FVector GetVolumeOrigin() const { return VolumeOrigin; }
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	TObjectPtr<ALandscape> TargetLandscape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	TSoftObjectPtr<ALandscape> TargetLandscapeAsset;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionGridSize = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionRegionSize = 16;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	int32 QuadsPerSection;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	FIntPoint TotalLandscapeComponentSize;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	int32 ComponentCountX;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	int32 ComponentCountY;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	int32 QuadsPerComponent;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	int32 SizeX;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	int32 SizeY;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	FVector VolumeExtent;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	FVector VolumeOrigin;	
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	TArray<ARuntimeVirtualTextureVolume*> CachedRuntimeVirtualTextureVolumes;
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache")
	TArray<TSoftObjectPtr<ARuntimeVirtualTextureVolume>> CachedRuntimeVirtualTextureVolumeAssets;
public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscapeInEditor();
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscape(UWorld* World);
	
private:
	void ImportMapDatas(UWorld* World, TArray<FLandscapeImportLayerInfo> ImportLayers);

	void InitializeLandscapeSetting(const UWorld* World);

	static void AddTargetLayers(ALandscape* Landscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	void ManageLandscapeRegions(UWorld* World, ALandscape* Landscape);

	static void AddLandscapeComponent(ULandscapeInfo* InLandscapeInfo, ULandscapeSubsystem* InLandscapeSubsystem, const TArray<FIntPoint>& InComponentCoordinates, TArray<ALandscapeProxy*>& OutCreatedStreamingProxies);

	static ALocationVolume* CreateLandscapeRegionVolume(UWorld* InWorld, ALandscapeProxy* InParentLandscapeActor, const FIntPoint& InRegionCoordinate, double InRegionSize);

	static void ForEachComponentByRegion(int32 RegionSize, const TArray<FIntPoint>& ComponentCoordinates, const TFunctionRef<bool(const FIntPoint&, const TArray<FIntPoint>&)>& RegionFn);

	void ForEachRegion_LoadProcessUnload(ULandscapeInfo* InLandscapeInfo, const FIntRect& InDomain, const UWorld* InWorld, const TFunctionRef<bool(const FBox&, const TArray<ALandscapeProxy*>)>& InRegionFn);
	
	template<typename T>
	void SaveObjects(TArrayView<T*> InObjects)
	{
		TArray<UPackage*> Packages;
		Algo::Transform(InObjects, Packages, [](const UObject* InObject) { return InObject->GetPackage(); });
		UEditorLoadingAndSavingUtils::SavePackages(Packages, /* bOnlyDirty = */ false);
	}
	
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> PrepareLandscapeLayerData(ALandscape* InTargetLandscape, AOCGLevelGenerator* InLevelGenerator, const UMapPreset* InMapPreset) const;

	static ULandscapeLayerInfoObject* CreateLayerInfo(ALandscape* InLandscape, const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

	static ULandscapeLayerInfoObject* CreateLayerInfo(const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate = nullptr);

	FString LayerInfoSavePath = TEXT("/Game/Landscape/LayerInfos");
	
	AOCGLevelGenerator* GetLevelGenerator() const;

	bool CreateRuntimeVirtualTextureVolume(ALandscape* InLandscapeActor);

	static bool ChangeGridSize(const UWorld* InWorld, ULandscapeInfo* InLandscapeInfo, uint32 InNewGridSizeInComponents);

	static ALandscapeProxy* FindOrAddLandscapeStreamingProxy(UActorPartitionSubsystem* InActorPartitionSubsystem, ULandscapeInfo* InLandscapeInfo, const UActorPartitionSubsystem::FCellCoord& InCellCoord);

	bool ShouldCreateNewLandscape(UMapPreset* InMapPreset);
	
	FVector GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const;

public:
	virtual void PostInitProperties() override;

protected:
	virtual void OnRegister() override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "RVT",  meta = (AllowPrivateAccess="true"))
	TObjectPtr<URuntimeVirtualTexture> ColorRVT = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "RVT",  meta = (AllowPrivateAccess="true"))
	TObjectPtr<URuntimeVirtualTexture> HeightRVT = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "RVT",  meta = (AllowPrivateAccess="true"))
	TObjectPtr<URuntimeVirtualTexture> DisplacementRVT = nullptr;

	UPROPERTY()
	TSoftObjectPtr<URuntimeVirtualTexture> ColorRVTAsset;
	UPROPERTY()
	TSoftObjectPtr<URuntimeVirtualTexture> HeightRVTAsset;
	UPROPERTY()
	TSoftObjectPtr<URuntimeVirtualTexture> DisplacementRVTAsset;
private:
	float CachedGlobalMinTemp;
	float CachedGlobalMaxTemp;
	float CachedGlobalMinHumidity;
	float CachedGlobalMaxHumidity;
	float LandscapeZScale;
	float LandscapeZOffset;
};
