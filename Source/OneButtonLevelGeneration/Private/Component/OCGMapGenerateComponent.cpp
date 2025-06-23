// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/OCGMapGenerateComponent.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "OCGLevelGenerator.h"
#include "Data/MapData.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"


// Sets default values for this component's properties
UOCGMapGenerateComponent::UOCGMapGenerateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UOCGMapGenerateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UOCGMapGenerateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

AOCGLevelGenerator* UOCGMapGenerateComponent::GetLevelGenerator() const
{
    return Cast<AOCGLevelGenerator>(GetOwner());
}

void UOCGMapGenerateComponent::GenerateMaps()
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    
    // Biomes.Empty();
    // for (const FOCGBiomeSettings& Biome : BiomesArr)
    // {
    //     Biomes.Add(Biome.BiomeName, Biome);
    // }

	const FRandomStream Stream(MapPreset->Seed);
    
	NoiseOffset.X = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
	NoiseOffset.Y = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);

    const FIntPoint CurMapResolution = MapPreset->MapResolution;
    HeightMapData.AddUninitialized(CurMapResolution.X * CurMapResolution.Y);
    
    // 2. 하이트맵 데이터 채우기
    for (int32 y = 0; y < CurMapResolution.Y; ++y)
    {
        for (int32 x = 0; x <CurMapResolution.X; ++x)
        {
            const float CalculatedHeight = CalculateHeightForCoordinate(x, y);
            const float NormalizedHeight = 32768.0f + CalculatedHeight;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            HeightMapData[y * CurMapResolution.X + x] = HeightValue;
        }
    }
    ExportMap(HeightMapData, "HeightMap.png");
    
    GenerateTempMap(HeightMapData, TemperatureMapData);

    GenerateHumidityMap(HeightMapData, TemperatureMapData, HumidityMapData);
    
    DecideBiome(HeightMapData, TemperatureMapData, HumidityMapData);
}

FIntPoint UOCGMapGenerateComponent::FixToNearestValidResolution(const FIntPoint InResolution)
{
    auto Fix = [](int32 Value) {
        int32 Pow = FMath::RoundToInt(FMath::Log2(static_cast<float>(Value - 1)));
        return FMath::Pow(2.f, static_cast<float>(Pow)) + 1;
    };

    return FIntPoint(Fix(InResolution.X), Fix(InResolution.Y));
}

void UOCGMapGenerateComponent::GenerateHeightMap(TArray<uint16>& OutHeightMap) const
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    
    const FIntPoint CurResolution = MapPreset->MapResolution;//FixToNearestValidResolution(MapResolution);
    OutHeightMap.AddZeroed(CurResolution.X * CurResolution.Y);
    
    // 2. 하이트맵 데이터 채우기
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            float CalculatedHeight = CalculateHeightForCoordinate(x, y);
            const float NormalizedHeight = 32768.0f + CalculatedHeight;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            OutHeightMap[y * CurResolution.X + x] = HeightValue;
        }
    }

    ExportMap(OutHeightMap, "HeightMap.png");
}

float UOCGMapGenerateComponent::CalculateHeightForCoordinate(const int32 InX, const int32 InY) const
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return 0.0f;

    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

	const float HeightRange = MapPreset->MaxHeight - MapPreset->MinHeight;
	// ==========================================================
    //            1. 대륙의 기반이 되는 저주파 노이즈 생성
    // ==========================================================
    float NoiseScale = 1;
    if (MapPreset->ApplyScaleToNoise && MapPreset->LandscapeScale > 1)
        NoiseScale = FMath::LogX(50.f, MapPreset->LandscapeScale) + 1;
    const float ContinentNoiseInputX = InX * MapPreset->ContinentNoiseScale * NoiseScale + NoiseOffset.X;
    const float ContinentNoiseInputY = InY * MapPreset->ContinentNoiseScale * NoiseScale + NoiseOffset.Y;
    // ContinentNoise는 0~1 사이의 값으로, 평야(0)와 산맥 기반(1)을 나눈다.
    const float ContinentNoise = FMath::PerlinNoise2D(FVector2D(ContinentNoiseInputX, ContinentNoiseInputY)) * 0.5f + 0.5f;
    // ==========================================================
    //            2. 지형 디테일을 위한 프랙탈 노이즈 생성
    // ==========================================================
    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float TerrainNoise = 0.0f;
    float MaxPossibleAmplitude = 0.0f;
    
    for (int32 i = 0; i < MapPreset->Octaves; ++i)
    {
        const float NoiseInputX = (InX * MapPreset->HeightNoiseScale * Frequency) + NoiseOffset.X;
        const float NoiseInputY = (InY * MapPreset->HeightNoiseScale * Frequency) + NoiseOffset.Y;
        const float PerlinValue = FMath::PerlinNoise2D(FVector2D(NoiseInputX, NoiseInputY)); // -1 ~ 1
        TerrainNoise += PerlinValue * Amplitude;
        MaxPossibleAmplitude += Amplitude;
        Amplitude *= MapPreset->Persistence;
        Frequency *= MapPreset->Lacunarity;
    }
    
    // 지형 노이즈를 -1 ~ 1 사이로 정규화
    TerrainNoise /= MaxPossibleAmplitude;
    
    // ==========================================================
    //         3. 두 노이즈를 지능적으로 결합
    // ==========================================================
    // 대륙 노이즈 값이 클수록(산맥 기반 지역), 지형 노이즈의 영향을 더 많이 받도록 한다.
    // ContinentInfluence로 그 강도를 조절.
    const float CombinedNoise = TerrainNoise * FMath::Pow(ContinentNoise, MapPreset->ContinentInfluence);
    
    // 최종 노이즈 값을 0~1 범위로 변환
    const float FinalNoiseValue = CombinedNoise * 0.5f + 0.5f;
    
    // 최종 높이 계산
    float Height = FinalNoiseValue;

    if (MapPreset->RedistributionFactor > 1.f && Height > 0.f && Height < 1.f)
    {
        const float PowX = FMath::Pow(Height, MapPreset->RedistributionFactor);
        const float Pow1_X = FMath::Pow(1-Height, MapPreset->RedistributionFactor);
        Height = PowX/(PowX + Pow1_X);
    }
    
    Height = FMath::Clamp(Height, 0.0f, 1.0f);
    
    // ==========================================================
    //         4. 최종 월드 높이로 변환
    // ==========================================================

    const float FinalWorldHeight = MapPreset->MinHeight + (Height * HeightRange);
    
    return FinalWorldHeight;
}

void UOCGMapGenerateComponent::GenerateTempMap(const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap)
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

    const FIntPoint CurResolution = MapPreset->MapResolution;//FixToNearestValidResolution(MapResolution);
	if (OutTempMap.Num() != CurResolution.X * CurResolution.Y)
    {
        OutTempMap.SetNumUninitialized(CurResolution.X * CurResolution.Y);
    }
    
    TArray<float> TempMapFloat;
    TempMapFloat.SetNumUninitialized(CurResolution.X * CurResolution.Y);

    float GlobalMinTemp = TNumericLimits<float>::Max();
    float GlobalMaxTemp = TNumericLimits<float>::Lowest();
    float TempRange = MapPreset->MaxTemp - MapPreset->MinTemp;

    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X + x;
            
            // ==========================================================
            //       ★ 1. 노이즈 기반으로 기본 온도 설정 ★
            // ==========================================================
            // 매우 낮은 주파수의 노이즈를 사용하여 따뜻한 지역과 추운 지역의 큰 덩어리를 만듭니다.
            // TODO : 0.002f Property로 빼기
            float TempNoiseInputX = x * 0.002f + NoiseOffset.X;
            float TempNoiseInputY = y * 0.002f + NoiseOffset.Y;

            // PerlinNoise2D는 -1~1, 이를 0~1로 변환하여 보간의 알파 값으로 사용합니다.
            float TempNoiseAlpha = FMath::PerlinNoise2D(FVector2D(TempNoiseInputX, TempNoiseInputY)) * 0.5f + 0.5f;

            // 노이즈 값에 따라 MinTemp와 MaxTemp 사이의 기본 온도를 결정합니다.
            float BaseTemp = FMath::Lerp(MapPreset->MinTemp, MapPreset->MaxTemp, TempNoiseAlpha);

            // ==========================================================
            //                 2. 고도에 따른 온도 감쇠 (그대로 유지)
            // ==========================================================
            const uint16 Height16 = InHeightMap[Index];
            const float WorldHeight = static_cast<float>(Height16) - 32768.0f;
            const float SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
            
            if (WorldHeight > SeaLevelHeight)
            {
                BaseTemp -= (WorldHeight / 1000.0f) * MapPreset->TempDropPer1000Units;
            }

            float NormalizedBaseTemp = (BaseTemp - MapPreset->MinTemp) / TempRange;
            if (MapPreset->RedistributionFactor > 1.f && NormalizedBaseTemp > 0.f && NormalizedBaseTemp < 1.f)
            {
                const float PowX = FMath::Pow(NormalizedBaseTemp, MapPreset->RedistributionFactor);
                const float Pow1_X = FMath::Pow(1-NormalizedBaseTemp, MapPreset->RedistributionFactor);
                NormalizedBaseTemp = PowX/(PowX + Pow1_X);
            }
            BaseTemp = MapPreset->MinTemp + NormalizedBaseTemp * TempRange;
            // ==========================================================
            //          3. 최종 온도 값 저장
            // ==========================================================
            const float FinalTemp = FMath::Clamp(BaseTemp, MapPreset->MinTemp, MapPreset->MaxTemp);
            
            TempMapFloat[Index] = FinalTemp;

            //if (FinalTemp > MinTemp + TempRange * 0.8f || FinalTemp < MinTemp + TempRange * 0.2f)
            //    UE_LOG(LogTemp, Log, TEXT("Temperature: %f"), FinalTemp);
            
            if (FinalTemp < GlobalMinTemp) GlobalMinTemp = FinalTemp;
            if (FinalTemp > GlobalMaxTemp) GlobalMaxTemp = FinalTemp;
        }
    }

    CachedGlobalMinTemp = GlobalMinTemp;
    CachedGlobalMaxTemp = GlobalMaxTemp;

    // ==========================================================
    //      4. float 온도맵을 uint16 이미지 데이터로 변환
    // ==========================================================
    TempRange = GlobalMaxTemp - GlobalMinTemp;
    if (TempRange < KINDA_SMALL_NUMBER)
    {
        TempRange = 1.0f; // 0으로 나누기 방지
    }

    for (int32 i = 0; i < TempMapFloat.Num(); ++i)
    {
        // 각 픽셀의 온도를 0~1 사이로 정규화합니다.
        const float NormalizedTemp = (TempMapFloat[i] - GlobalMinTemp) / TempRange;
        
        // 0~1 값을 0~65535 범위의 uint16 값으로 변환합니다.
        OutTempMap[i] = static_cast<uint16>(NormalizedTemp * 65535.0f);
    }

    // 이미지 파일로 저장하여 결과 확인
    ExportMap(OutTempMap, "TempMap.png");
}

void UOCGMapGenerateComponent::GenerateHumidityMap(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
	TArray<uint16>& OutHumidityMap)
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    const FIntPoint CurResolution = MapPreset->MapResolution;//FixToNearestValidResolution(MapResolution);
    if (OutHumidityMap.Num() != CurResolution.X * CurResolution.Y)
    {
        OutHumidityMap.SetNumUninitialized(CurResolution.X * CurResolution.Y);
    }
    
    // --- 사전 준비 ---
    const float SeaLevelWorldHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);

    // 임시 float 배열들을 사용하여 중간 계산 결과를 저장합니다.
    TArray<float> HumidityMapFloat;
    HumidityMapFloat.Init(0.0f, CurResolution.X * CurResolution.Y);

    // 각 픽셀에서 가장 가까운 물까지의 거리를 저장할 배열
    TArray<float> DistanceToWater;
    DistanceToWater.Init(TNumericLimits<float>::Max(), CurResolution.X * CurResolution.Y);
    
    // ==========================================================
    //            Pass 1: 물 공급원 찾기 및 거리 계산
    // ==========================================================
    
    // 1-A: 모든 물 픽셀(습기 공급원)을 찾아서 큐에 넣습니다.
    TQueue<FIntPoint> Frontier; // 너비 우선 탐색(BFS)을 위한 큐
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X  + x;
            const float WorldHeight = (static_cast<float>(InHeightMap[Index]) - 32768.0f); // Z스케일 문제 무시

            if (WorldHeight <= SeaLevelWorldHeight)
            {
                DistanceToWater[Index] = 0; // 물 픽셀은 거리가 0
                Frontier.Enqueue(FIntPoint(x, y));
            }
        }
    }

    // 1-B: BFS를 사용하여 모든 육지 픽셀에서 가장 가까운 물까지의 거리를 계산합니다.
    const int32 dx[] = {0, 0, 1, -1};
    const int32 dy[] = {1, -1, 0, 0};

    while (!Frontier.IsEmpty())
    {
        FIntPoint Current;
        Frontier.Dequeue(Current);

        for (int32 i = 0; i < 4; ++i)
        {
            const int32 nx = Current.X + dx[i];
            const int32 ny = Current.Y + dy[i];
            
            if (nx >= 0 && nx < CurResolution.X && ny >= 0 && ny < CurResolution.Y)
            {
                const int32 NeighborIndex = ny * CurResolution.X + nx;
                // 아직 방문하지 않은(거리가 무한대인) 이웃 픽셀이라면
                if (DistanceToWater[NeighborIndex] == TNumericLimits<float>::Max())
                {
                    DistanceToWater[NeighborIndex] = DistanceToWater[Current.Y * CurResolution.X + Current.X] + 1.0f;
                    Frontier.Enqueue(FIntPoint(nx, ny));
                }
            }
        }
    }

    // ==========================================================
    //       Pass 2: 거리와 온도를 기반으로 최종 습도 결정
    // ==========================================================
    float GlobalMinHumidity = TNumericLimits<float>::Max();
    float GlobalMaxHumidity = TNumericLimits<float>::Lowest();
    
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        float FinalHumidity = 0.0f;
        
        if (DistanceToWater[i] == 0)
        {
            // 이 픽셀은 물이므로, 온도의 영향을 무시하고 습도를 최대로 설정합니다.
            FinalHumidity = 1.0f; 
        }
        else
        {
            // 2-A: 물과의 거리를 기반으로 기본 습도를 계산합니다.
            const float HumidityFromDistance = FMath::Exp(-DistanceToWater[i] * MapPreset->MoistureFalloffRate);
        
            // 2-B: 온도의 영향을 적용합니다.
            const float NormalizedTemp = static_cast<float>(InTempMap[i]) / 65535.0f;
            FinalHumidity = HumidityFromDistance * (1.0f - (NormalizedTemp * MapPreset->TemperatureInfluenceOnHumidity));
        }

        FinalHumidity = FMath::Clamp(FinalHumidity, 0.0f, 1.0f);
        
        if (MapPreset->RedistributionFactor > 1.f && FinalHumidity > 0.f && FinalHumidity < 1.f)
        {
            const float PowX = FMath::Pow(FinalHumidity, MapPreset->RedistributionFactor);
            const float Pow1_X = FMath::Pow(1-FinalHumidity, MapPreset->RedistributionFactor);
            FinalHumidity = PowX/(PowX + Pow1_X);
        }
        
        HumidityMapFloat[i] = FinalHumidity;
        
        // 최종 uint16 변환을 위해 최소/최대값 추적
        if (FinalHumidity < GlobalMinHumidity) GlobalMinHumidity = FinalHumidity;
        if (FinalHumidity > GlobalMaxHumidity) GlobalMaxHumidity = FinalHumidity;
    }

    CachedGlobalMinHumidity = GlobalMinHumidity;
    CachedGlobalMaxHumidity = GlobalMaxHumidity;
    
    // ==========================================================
    //      3. float 습도맵을 uint16 이미지 데이터로 변환
    // ==========================================================
    float HumidityRange = GlobalMaxHumidity - GlobalMinHumidity;
    if (HumidityRange < KINDA_SMALL_NUMBER) HumidityRange = 1.0f;

    for (int32 i = 0; i < HumidityMapFloat.Num(); ++i)
    {
        const float NormalizedHumidity = (HumidityMapFloat[i] - GlobalMinHumidity) / HumidityRange;
        OutHumidityMap[i] = static_cast<uint16>(NormalizedHumidity * 65535.0f);
    }

    ExportMap(OutHumidityMap, "HumidityMap.png");
}

void UOCGMapGenerateComponent::DecideBiome(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
    const TArray<uint16>& InHumidityMap)
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    const FIntPoint CurResolution = MapPreset->MapResolution;//FixToNearestValidResolution(MapResolution);
    
    TArray<FColor> BiomeColorMap;
    BiomeColorMap.AddUninitialized(CurResolution.X * CurResolution.Y);
    TArray<FName> BiomeMap;
    BiomeMap.AddUninitialized(CurResolution.X * CurResolution.Y);

    for (auto& Biome : MapPreset->Biomes)
    {
        TArray<uint8> WeightLayer;
        WeightLayer.SetNumZeroed(CurResolution.X * CurResolution.Y);
        WeightLayers.Add(Biome.Key, WeightLayer);
    }

    const float SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
    
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X + x;
            float Height = static_cast<float>(InHeightMap[Index]);
            Height -= 32768.f;
            float NormalizedTemp = static_cast<float>(InTempMap[y * CurResolution.X + x]) / 65535.f;
            const float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y * CurResolution.X + x]) / 65535.f;
            const float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            if (y == CurResolution.Y / 2 && x == CurResolution.X / 2)
                UE_LOG(LogTemp, Display, TEXT("Temp : %f, Humidity : %f"), Temp, Humidity);
            const FOCGBiomeSettings* CurrentBiome = nullptr;
            if (Height <= SeaLevelHeight)
            {
                CurrentBiome = MapPreset->Biomes.Find(TEXT("Water"));
            }
            else
            {
                float MinDist = TNumericLimits<float>::Max();
                float TempRange = MapPreset->MaxTemp - MapPreset->MinTemp;
                for (auto& Biome : MapPreset->Biomes)
                {
                    if (Biome.Value.BiomeName == TEXT("Water"))
                        continue;
                    float TempDiff = FMath::Abs(Biome.Value.Temperature - Temp);
                    float HumidityDiff = FMath::Abs(Biome.Value.Humidity - Humidity);
                    HumidityDiff = (MapPreset->MaxTemp - MapPreset->MinTemp) * HumidityDiff;
                    float Dist = FVector2D(TempDiff, HumidityDiff).Length();
                    if (Dist < MinDist)
                    {
                        MinDist = Dist;
                        CurrentBiome = &Biome.Value;
                    }
                }
            }
            
            if(CurrentBiome)
            {
                WeightLayers[CurrentBiome->BiomeName][Index] = 255;
                // CurrentBiome->WeightLayer[Index] = 255;
                BiomeColorMap[Index] = CurrentBiome->Color.ToFColor(true);
                BiomeMap[Index] = CurrentBiome->BiomeName;
            }
        }
    }
    MapDataUtils::ExportMap(BiomeColorMap, CurResolution, "BiomeMap0.png");
    BelndBiome(BiomeMap);
}

void UOCGMapGenerateComponent::BelndBiome(const TArray<FName>& InBiomeMap)
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

    const FIntPoint CurResolution = MapPreset->MapResolution;
     // 초기화 및 원본 맵 복사
    TMap<FName, TArray<uint8>> OriginalWeightMaps;
    for (auto& Layer : WeightLayers)
    {
        Layer.Value.Init(0, CurResolution.X * CurResolution.Y);
        
        TArray<uint8> InitialWeights;
        InitialWeights.Init(0, CurResolution.X * CurResolution.Y);

        OriginalWeightMaps.FindOrAdd(Layer.Key, InitialWeights);
    }
    
    // for (auto& Biome : Biomes)
    // {
    //     Biome.Value.WeightLayer.Init(0, CurResolution.X * CurResolution.Y); // 최종 결과를 저장할 배열 초기화
    //     
    //     TArray<uint8> InitialWeights;
    //     InitialWeights.Init(0, CurResolution.X * CurResolution.Y);
    //     OriginalWeightMaps.Add(Biome.Value.BiomeName, InitialWeights);
    // }

    // 칼같이 나뉘는 초기 웨이트맵 생성 (복사본에)
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        if (OriginalWeightMaps.Contains(InBiomeMap[i]))
        {
            OriginalWeightMaps[InBiomeMap[i]][i] = 255;
        }
    }
    
    // 경계 처리를 포함한 블러 필터 적용
    // 수평 블러 결과를 저장할 임시 맵
    TMap<FName, TArray<float>> HorizontalPassMaps;
    for (const auto& Elem : OriginalWeightMaps)
    {
        HorizontalPassMaps.Add(Elem.Key, TArray<float>());
        HorizontalPassMaps.FindChecked(Elem.Key).Init(0.f, CurResolution.X * CurResolution.Y);
    }
    
    const int32 BlendRadius = MapPreset->BiomeBlendRadius;

    // 수평 블러 (슬라이딩 윈도우)
    for (const auto& Elem : OriginalWeightMaps)
    {
        const FName& BiomeName = Elem.Key;
        const TArray<uint8>& OriginalLayer = Elem.Value;
        TArray<float>& HorizontalPassLayer = HorizontalPassMaps.FindChecked(BiomeName);

        for (int32 y = 0; y < CurResolution.Y; ++y)
        {
            float Sum = 0;
            // 첫 픽셀의 윈도우 합계 계산
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
            {
                int32 SampleX = FMath::Clamp(i, 0, CurResolution.Y - 1);
                Sum += OriginalLayer[y * CurResolution.X  + SampleX];
            }
            HorizontalPassLayer[y * CurResolution.X  + 0] = Sum;

            // 슬라이딩 윈도우
            for (int32 x = 1; x < CurResolution.X ; ++x)
            {
                int32 OldX = FMath::Clamp(x - BlendRadius - 1, 0, CurResolution.X  - 1);
                int32 NewX = FMath::Clamp(x + BlendRadius, 0, CurResolution.X  - 1);
                Sum += OriginalLayer[y * CurResolution.X  + NewX] - OriginalLayer[y * CurResolution.X + OldX];
                HorizontalPassLayer[y * CurResolution.X  + x] = Sum;
            }
        }
    }

    // 수직 블러 (슬라이딩 윈도우)
    const float BlendFactor = 1.f / ((BlendRadius * 2 + 1) * (BlendRadius * 2 + 1));
    for (const auto& Elem : HorizontalPassMaps)
    {
        const FName& BiomeName = Elem.Key;
        const TArray<float>& HorizontalPassLayer = Elem.Value;
        TArray<uint8>& FinalLayer = *WeightLayers.Find(BiomeName);

        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            float Sum = 0;
            // 첫 픽셀의 윈도우 합계 계산
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
            {
                int32 SampleY = FMath::Clamp(i, 0, CurResolution.X - 1);
                Sum += HorizontalPassLayer[SampleY * CurResolution.X  + x];
            }
            FinalLayer[x] = FMath::RoundToInt(Sum * BlendFactor);

            // 슬라이딩 윈도우
            for (int32 y = 1; y < CurResolution.Y; ++y)
            {   
                int32 OldY = FMath::Clamp(y - BlendRadius - 1, 0, CurResolution.X - 1);
                int32 NewY = FMath::Clamp(y + BlendRadius, 0, CurResolution.X - 1);
                Sum += HorizontalPassLayer[NewY * CurResolution.X + x] - HorizontalPassLayer[OldY * CurResolution.X + x];
                FinalLayer[y * CurResolution.X + x] = FMath::RoundToInt(Sum * BlendFactor);
            }
        }
    }
    
    // 각 픽셀에 대해 모든 바이옴의 블러링된 웨이트 합이 255가 되도록 보정
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        float TotalWeight = 0;
        for (auto& Layer : WeightLayers)
        {
            TotalWeight += Layer.Value[i];
        }

        if (TotalWeight > 0)
        {
            float NormalizationFactor = 255.f / TotalWeight;
            for (auto& Layer : WeightLayers)
            {
                Layer.Value[i] = FMath::RoundToInt(Layer.Value[i] * NormalizationFactor);
            }
        }
    }
}

void UOCGMapGenerateComponent::ExportMap(const TArray<uint16>& InMap, const FString& FileName) const
{
#if WITH_EDITOR
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();

    const FIntPoint CurResolution = MapPreset->MapResolution;
    
     // 1. 저장할 파일 경로를 동적으로 생성합니다.
    // 프로젝트의 콘텐츠 폴더 경로를 가져옵니다. (예: "C:/MyProject/Content/")
    const FString ContentDir = FPaths::ProjectContentDir();
    
    // 하위 폴더와 파일 이름을 지정합니다. 시드 값을 파일 이름에 포함시켜 구별하기 쉽게 만듭니다.
    const FString SubDir = TEXT("Maps/");
    
    // 디렉토리 경로를 먼저 조합합니다.
    const FString DirectoryPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/"));

    // 디렉토리가 존재하지 않으면 생성합니다.
    if (!FPaths::DirectoryExists(DirectoryPath))
    {
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.CreateDirectoryTree(*DirectoryPath))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *DirectoryPath);
            return;
        }
    }

    // 최종 파일 경로를 완성합니다.
    const FString FullPath = FPaths::Combine(DirectoryPath, FileName);
    
    UE_LOG(LogTemp, Log, TEXT("Attempting to export heightmap to: %s"), *FullPath);

    // 2. 이미지 래퍼(Image Wrapper) 모듈을 로드합니다.
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    
    // PNG 형식으로 저장할 래퍼를 생성합니다.
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper."));
        return;
    }

    // 3. 이미지 래퍼에 데이터 설정
    // 16비트 그레이스케일(G16) 형식으로 데이터를 설정합니다.
    if (ImageWrapper->SetRaw(InMap.GetData(), InMap.GetAllocatedSize(), CurResolution.X, CurResolution.Y, ERGBFormat::Gray, 16))
    {
        // 4. 데이터를 압축하여 TArray<uint8> 버퍼로 가져옵니다.
        const TArray64<uint8>& PngData = ImageWrapper->GetCompressed(100);

        // 5. 파일로 저장합니다.
        if (FFileHelper::SaveArrayToFile(PngData, *FullPath))
        {
            UE_LOG(LogTemp, Log, TEXT("map exported successfully to: %s"), *FullPath);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to save map file."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to set raw data to Image Wrapper."));
    }
#endif
}
