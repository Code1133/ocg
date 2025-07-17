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

USTRUCT(BlueprintType)
struct FLandscapeSetting
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 WorldPartitionGridSize = 2;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 WorldPartitionRegionSize = 16;
	
	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	uint32 QuadsPerSection = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	FIntPoint TotalLandscapeComponentSize = FIntPoint::ZeroValue;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 ComponentCountX = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 ComponentCountY = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 QuadsPerComponent = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 SizeX = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	int32 SizeY = 0;

	// == 연산자 오버로드
	bool operator==(FLandscapeSetting const& Other) const
	{
		return WorldPartitionGridSize		 == Other.WorldPartitionGridSize
			&& WorldPartitionRegionSize		 == Other.WorldPartitionRegionSize
			&& QuadsPerSection               == Other.QuadsPerSection
			&& TotalLandscapeComponentSize   == Other.TotalLandscapeComponentSize
			&& ComponentCountX               == Other.ComponentCountX
			&& ComponentCountY               == Other.ComponentCountY
			&& QuadsPerComponent             == Other.QuadsPerComponent
			&& SizeX                         == Other.SizeX
			&& SizeY                         == Other.SizeY;
	}

	bool operator!=(FLandscapeSetting const& Other) const
	{
		return !(*this == Other);
	}
};

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
	ALandscape* GetLandscape();

	void SetLandscapeZValues(float ZScale, float ZOffset) { LandscapeZScale = ZScale; LandscapeZOffset =  ZOffset; }
	FVector GetVolumeExtent() const { return VolumeExtent; }
	FVector GetVolumeOrigin() const { return VolumeOrigin; }
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	TObjectPtr<ALandscape> TargetLandscape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	TSoftObjectPtr<ALandscape> TargetLandscapeAsset;
private:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Landscape|Cache", meta = (AllowPrivateAccess = "true"))
	FLandscapeSetting LandscapeSetting;
	
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
	void InitializeLandscapeSetting(const UWorld* World);
	
	AOCGLevelGenerator* GetLevelGenerator() const;

	bool CreateRuntimeVirtualTextureVolume(ALandscape* InLandscapeActor);
	
	bool ShouldCreateNewLandscape(const UWorld* World);

	static bool IsLandscapeSettingChanged(const FLandscapeSetting& Prev, const FLandscapeSetting& Curr);
	
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
