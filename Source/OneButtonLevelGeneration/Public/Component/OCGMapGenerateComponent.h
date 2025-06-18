// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGMapGenerateComponent.generated.h"

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
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivateAccess="true"))
	TMap<FName, FOCGBiomeSettings> Biomes;
		
	UPROPERTY()
	TArray<uint16> HeightMapData;
	
	UPROPERTY()
	TArray<uint16> TemperatureMapData;
	
	UPROPERTY()
	TArray<uint16> HumidityMapData;

public:
	FORCEINLINE const TArray<uint16>& GetHeightMapData() { return HeightMapData; }
	FORCEINLINE const TArray<uint16>& GetTemperatureMapData() { return TemperatureMapData; }
	FORCEINLINE const TArray<uint16>& GetHumidityMapData() { return HumidityMapData; }

	FORCEINLINE const TMap<FName, FOCGBiomeSettings>& GetBiomes() { return Biomes; }

	FORCEINLINE const FIntPoint& GetMapResolution() const { return MapResolution; }
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
	int32 Seed = 1337;
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (ClampMin = "63", ClampMax = "8129", UIMin = "63", UIMax = "8129", AllowPrivateAccess="true"))
	FIntPoint MapResolution = FIntPoint(1009, 1009);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
	TArray<UTexture2D*> AdditiveHeightMaps;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (ClampMin = -32768.f, AllowPrivateAccess="true"))
	float MinHeight = -15000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (ClampMax = 32768.f, AllowPrivateAccess="true"))
	float MaxHeight = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (ClampMin = "0.0", ClampMax = "1.0", AllowPrivateAccess="true") )
	float SeaLevel = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
    float MinTemp = -30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
    float MaxTemp = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
	float TempDropPer1000Units = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (ClampMin = "0.0", AllowPrivateAccess="true"))
    float MoistureFalloffRate = 0.0005f;

    // 온도가 습도에 미치는 영향 (클수록 더운 지역이 더 건조해짐)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (ClampMin = "0.0", AllowPrivateAccess="true"))
    float TemperatureInfluenceOnHumidity = 0.7f;

	UPROPERTY()
	FVector2D NoiseOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
	float StandardNoiseOffset = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
	float RedistributionFactor = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Settings", meta = (AllowPrivateAccess="true"))
	int32 BiomeBlendRadius = 10;
private:
	// --- Noise Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (DisplayName = "Terrain Noise Scale", AllowPrivateAccess="true"))
	float HeightNoiseScale = 0.002f;

	// 대륙의 큰 형태를 만드는 저주파 노이즈 스케일
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Procedural Settings|Noise", meta = (AllowPrivateAccess="true"))
	float ContinentNoiseScale = 0.002f;

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
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateMaps();

private:
	static FIntPoint FixToNearestValidResolution(FIntPoint InResolution);

	void GenerateHeightMap(TArray<uint16>& OutHeightMap) const;
	float CalculateHeightForCoordinate(const int32 InX, const int32 InY) const;
	void GenerateTempMap(const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap);
	void GenerateHumidityMap(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap, TArray<uint16>& OutHumidityMap);
	void DecideBiome(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap, const TArray<uint16>& InHumidityMap);
	void BelndBiome(const TArray<FName>& InBiomeMap);
	void ExportMap(const TArray<uint16>& InMap, const FString& FileName) const;

private:
	float CachedGlobalMinTemp;
	float CachedGlobalMaxTemp;
	float CachedGlobalMinHumidity;
	float CachedGlobalMaxHumidity;
};
