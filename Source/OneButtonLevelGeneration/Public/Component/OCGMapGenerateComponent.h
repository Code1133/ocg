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
public:
	FORCEINLINE const TArray<uint16>& GetHeightMapData() { return HeightMapData; }
	FORCEINLINE const TArray<uint16>& GetTemperatureMapData() { return TemperatureMapData; }
	FORCEINLINE const TArray<uint16>& GetHumidityMapData() { return HumidityMapData; }
	FORCEINLINE const TMap<FName, TArray<uint8>>& GetWeightLayers() { return WeightLayers; }

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

	void GenerateHeightMap(TArray<uint16>& OutHeightMap) const;
	float CalculateHeightForCoordinate(const int32 InX, const int32 InY) const;
	void GenerateTempMap(const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap);
	void GenerateHumidityMap(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap, TArray<uint16>& OutHumidityMap);
	void DecideBiome(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap, const TArray<uint16>& InHumidityMap, TArray<const FOCGBiomeSettings*>& OutBiomeMap);
	void BlendBiome(const TArray<FName>& InBiomeMap);
	void ExportMap(const TArray<uint16>& InMap, const FString& FileName) const;
	void ErosionPass(TArray<uint16>& InOutHeightMap);
	void InitializeErosionBrush();
	float CalculateHeightAndGradient(const TArray<float>& HeightMap, float PosX, float PosY, FVector2D& OutGradient);
	void ApplyBiome(TArray<uint16>& InOutHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap);
	void CaculateBiomeAverageHeights(const TArray<const FOCGBiomeSettings*>& InBiomeMap, TArray<float>& OutAverageHeights);
	void GetBiomeStats(FIntPoint MapSize, uint32 x, uint32 y, uint32 RegionID, float& OutTotalHeight, uint32& OutTotalPixel, TArray<uint32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<FOCGBiomeSettings*>& InBiomeMap);

private:
	float CachedGlobalMinTemp;
	float CachedGlobalMaxTemp;
	float CachedGlobalMinHumidity;
	float CachedGlobalMaxHumidity;
};
