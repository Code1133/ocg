// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGMapGenerateComponent.generated.h"

class AOCGLevelGenerator;
struct FOCGBiomeSettings;
struct FLandscapeImportLayerInfo;
class ALandscape;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONEBUTTONLEVELGENERATION_API UOCGMapGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOCGMapGenerateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	AOCGLevelGenerator* GetLevelGenerator() const;
	
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivateAccess="true"))
	// TMap<FName, FOCGBiomeSettings> Biomes;
	
	TArray<uint16> HeightMapData;
	TArray<uint16> TemperatureMapData;
	TArray<uint16> HumidityMapData;
	TMap<FName, TArray<uint8>> WeightLayers;
	TArray<FColor> BiomeColorMap;
public:
	FORCEINLINE const TArray<uint16>& GetHeightMapData() { return HeightMapData; }
	FORCEINLINE const TArray<uint16>& GetTemperatureMapData() { return TemperatureMapData; }
	FORCEINLINE const TArray<uint16>& GetHumidityMapData() { return HumidityMapData; }
	FORCEINLINE const TMap<FName, TArray<uint8>>& GetWeightLayers() { return WeightLayers; }
	FORCEINLINE const TArray<FColor>& GetBiomeColorMap() { return BiomeColorMap; }

private:
	UPROPERTY()
	FVector2D PlainNoiseOffset;
	FVector2D MountainNoiseOffset;
	FVector2D BlendNoiseOffset;
	FVector2D DetailNoiseOffset;
	FVector2D IslandNoiseOffset;
	float NoiseScale;
	float PlainHeight;
	FRandomStream Stream;
	//침식 관련 변수
	TArray<TArray<int32>> ErosionBrushIndices;
	TArray<TArray<float>> ErosionBrushWeights;
	int32 CurrentErosionRadius = 0;

public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateMaps();

private:
	static FIntPoint FixToNearestValidResolution(FIntPoint InResolution);

	void Initialize(const UMapPreset* MapPreset);
	void InitializeNoiseOffsets(const UMapPreset* MapPreset);
	void GenerateHeightMap(const UMapPreset* MapPreset, const FIntPoint CurMapResolution, TArray<uint16>& OutHeightMap);
	float CalculateHeightForCoordinate(const UMapPreset* MapPreset, const int32 InX, const int32 InY) const;
	void GenerateTempMap(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap);
	void GenerateHumidityMap(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap, TArray<uint16>& OutHumidityMap);
	void DecideBiome(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap, const TArray<uint16>& InHumidityMap, TArray<const FOCGBiomeSettings*>& OutBiomeMap);
	void BlendBiome(const UMapPreset* MapPreset, const TArray<FName>& InBiomeMap);
	void ExportMap(const UMapPreset* MapPreset, const TArray<uint16>& InMap, const FString& FileName) const;
	void ErosionPass(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap);
	void InitializeErosionBrush();
	float CalculateHeightAndGradient(const UMapPreset* MapPreset, const TArray<float>& HeightMap, float PosX, float PosY, FVector2D& OutGradient);
	void ModifyLandscapeWithBiome(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap);
	void CalculateBiomeAverageHeights(const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap, TArray<float>& OutAverageHeights, const UMapPreset* MapPreset);
	void BlurBiomeAverageHeights(TArray<float>& OutAverageHeights, const TArray<float>& InAverageHeights, const UMapPreset* MapPreset);
	void GetBiomeStats(FIntPoint MapSize, int32 x, int32 y, int32 RegionID, float& OutTotalHeight, int32& OutTotalPixel, TArray<int32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap);

private:
	float CachedGlobalMinTemp;
	float CachedGlobalMaxTemp;
	float CachedGlobalMinHumidity;
	float CachedGlobalMaxHumidity;
};
