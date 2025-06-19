// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/OCGMapGenerateComponent.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "OCGLevelGenerator.h"
#include "Data/MapData.h"
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
	FRandomStream Stream(Seed);
	NoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	NoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    
    HeightMapData.AddUninitialized(MapResolution.X * MapResolution.Y);
    
    // 2. 하이트맵 데이터 채우기
    for (int32 y = 0; y < MapResolution.Y; ++y)
    {
        for (int32 x = 0; x < MapResolution.X; ++x)
        {
            float CalculatedHeight = CalculateHeightForCoordinate(x, y);
            const float NormalizedHeight = 32768.0f + CalculatedHeight;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            HeightMapData[y * MapResolution.X  + x] = HeightValue;
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
    FIntPoint InResolution = MapResolution;//FixToNearestValidResolution(MapResolution);
    OutHeightMap.AddZeroed(InResolution.X * InResolution.Y);
    
    // 2. 하이트맵 데이터 채우기
    for (int32 y = 0; y < InResolution.Y; ++y)
    {
        for (int32 x = 0; x < InResolution.X; ++x)
        {
            float CalculatedHeight = CalculateHeightForCoordinate(x, y);
            const float NormalizedHeight = 32768.0f + CalculatedHeight;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            OutHeightMap[y * InResolution.X + x] = HeightValue;
        }
    }

    ExportMap(OutHeightMap, "HeightMap.png");
    MapDataUtils::ImportTextureFromPNG("HeightMap.png");
}

float UOCGMapGenerateComponent::CalculateHeightForCoordinate(const int32 InX, const int32 InY) const
{
	float HeightRange = MaxHeight - MinHeight;
	// ==========================================================
    //            1. 대륙의 기반이 되는 저주파 노이즈 생성
    // ==========================================================
    float ContinentNoiseInputX = InX * ContinentNoiseScale + NoiseOffset.X;
    float ContinentNoiseInputY = InY * ContinentNoiseScale + NoiseOffset.Y;
    // ContinentNoise는 0~1 사이의 값으로, 평야(0)와 산맥 기반(1)을 나눈다.
    float ContinentNoise = FMath::PerlinNoise2D(FVector2D(ContinentNoiseInputX, ContinentNoiseInputY)) * 0.5f + 0.5f;
    
    //return MinHeight + (ContinentNoise) * HeightRange;

    // ==========================================================
    //            2. 지형 디테일을 위한 프랙탈 노이즈 생성
    // ==========================================================
    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float TerrainNoise = 0.0f;
    float MaxPossibleAmplitude = 0.0f;
    
    for (int32 i = 0; i < Octaves; ++i)
    {
        float NoiseInputX = (InX * HeightNoiseScale * Frequency) + NoiseOffset.X;
        float NoiseInputY = (InY * HeightNoiseScale * Frequency) + NoiseOffset.Y;
        float PerlinValue = FMath::PerlinNoise2D(FVector2D(NoiseInputX, NoiseInputY)); // -1 ~ 1
        TerrainNoise += PerlinValue * Amplitude;
        MaxPossibleAmplitude += Amplitude;
        Amplitude *= Persistence;
        Frequency *= Lacunarity;
    }
    
    // 지형 노이즈를 -1 ~ 1 사이로 정규화
    TerrainNoise /= MaxPossibleAmplitude;
    
    // ==========================================================
    //         3. 두 노이즈를 지능적으로 결합
    // ==========================================================
    // 대륙 노이즈 값이 클수록(산맥 기반 지역), 지형 노이즈의 영향을 더 많이 받도록 한다.
    // ContinentInfluence로 그 강도를 조절.
    float CombinedNoise = TerrainNoise * FMath::Pow(ContinentNoise, ContinentInfluence);
    
    // 최종 노이즈 값을 0~1 범위로 변환
    float FinalNoiseValue = CombinedNoise * 0.5f + 0.5f;
    
    // 최종 높이 계산
    float Height = FinalNoiseValue;

    if (RedistributionFactor > 1.f && Height > 0.f && Height < 1.f)
    {
        float PowX = FMath::Pow(Height, RedistributionFactor);
        float Pow1_X = FMath::Pow(1-Height, RedistributionFactor);
        Height = PowX/(PowX + Pow1_X);
    }
    
    Height = FMath::Clamp(Height, 0.0f, 1.0f);
    
    // ==========================================================
    //         4. 최종 월드 높이로 변환
    // ==========================================================
    
    float FinalWorldHeight = MinHeight + (Height * HeightRange);
    
    return FinalWorldHeight;
}

void UOCGMapGenerateComponent::GenerateTempMap(const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap)
{
    FIntPoint InResolution = MapResolution;//FixToNearestValidResolution(MapResolution);
	if (OutTempMap.Num() != InResolution.X * InResolution.Y)
    {
        OutTempMap.SetNumUninitialized(InResolution.X * InResolution.Y);
    }
    
    TArray<float> TempMapFloat;
    TempMapFloat.SetNumUninitialized(InResolution.X * InResolution.Y);

    float GlobalMinTemp = TNumericLimits<float>::Max();
    float GlobalMaxTemp = TNumericLimits<float>::Lowest();
    float TempRange = MaxTemp - MinTemp;

    for (int32 y = 0; y < InResolution.Y; ++y)
    {
        for (int32 x = 0; x < InResolution.X; ++x)
        {
            const int32 Index = y * InResolution.X + x;
            
            // ==========================================================
            //       ★ 1. 노이즈 기반으로 기본 온도 설정 ★
            // ==========================================================
            // 매우 낮은 주파수의 노이즈를 사용하여 따뜻한 지역과 추운 지역의 큰 덩어리를 만듭니다.
            float TempNoiseInputX = x * 0.002f + NoiseOffset.X;
            float TempNoiseInputY = y * 0.002f + NoiseOffset.Y;

            // PerlinNoise2D는 -1~1, 이를 0~1로 변환하여 보간의 알파 값으로 사용합니다.
            float TempNoiseAlpha = FMath::PerlinNoise2D(FVector2D(TempNoiseInputX, TempNoiseInputY)) * 0.5f + 0.5f;

            // 노이즈 값에 따라 MinTemp와 MaxTemp 사이의 기본 온도를 결정합니다.
            float BaseTemp = FMath::Lerp(MinTemp, MaxTemp, TempNoiseAlpha);

            // ==========================================================
            //                 2. 고도에 따른 온도 감쇠 (그대로 유지)
            // ==========================================================
            const uint16 Height16 = InHeightMap[Index];
            const float WorldHeight = static_cast<float>(Height16) - 32768.0f;
            const float SeaLevelHeight = MinHeight + SeaLevel * (MaxHeight - MinHeight);
            
            if (WorldHeight > SeaLevelHeight)
            {
                BaseTemp -= (WorldHeight / 1000.0f) * TempDropPer1000Units;
            }
            else
            {
                BaseTemp += (WorldHeight / 1000.0f) * TempDropPer1000Units;
            }

            float NormalizedBaseTemp = (BaseTemp - MinTemp) / TempRange;
            if (RedistributionFactor > 1.f && NormalizedBaseTemp > 0.f && NormalizedBaseTemp < 1.f)
            {
                float PowX = FMath::Pow(NormalizedBaseTemp, RedistributionFactor);
                float Pow1_X = FMath::Pow(1-NormalizedBaseTemp, RedistributionFactor);
                NormalizedBaseTemp = PowX/(PowX + Pow1_X);
            }
            BaseTemp = MinTemp + NormalizedBaseTemp * TempRange;
            // ==========================================================
            //          3. 최종 온도 값 저장
            // ==========================================================
            float FinalTemp = FMath::Clamp(BaseTemp, MinTemp, MaxTemp);
            
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
    if (TempRange < KINDA_SMALL_NUMBER) TempRange = 1.0f; // 0으로 나누기 방지

    for (int32 i = 0; i < TempMapFloat.Num(); ++i)
    {
        // 각 픽셀의 온도를 0~1 사이로 정규화합니다.
        float NormalizedTemp = (TempMapFloat[i] - GlobalMinTemp) / TempRange;
        
        // 0~1 값을 0~65535 범위의 uint16 값으로 변환합니다.
        OutTempMap[i] = static_cast<uint16>(NormalizedTemp * 65535.0f);
    }

    // 이미지 파일로 저장하여 결과 확인
    ExportMap(OutTempMap, "TempMap.png");
}

void UOCGMapGenerateComponent::GenerateHumidityMap(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
	TArray<uint16>& OutHumidityMap)
{
    FIntPoint InResolution = MapResolution;//FixToNearestValidResolution(MapResolution);
    if (OutHumidityMap.Num() != InResolution.X * InResolution.Y)
    {
        OutHumidityMap.SetNumUninitialized(InResolution.X * InResolution.Y);
    }
    
    // --- 사전 준비 ---
    const float SeaLevelWorldHeight = MinHeight + SeaLevel * (MaxHeight - MinHeight);

    // 임시 float 배열들을 사용하여 중간 계산 결과를 저장합니다.
    TArray<float> HumidityMapFloat;
    HumidityMapFloat.Init(0.0f, InResolution.X * InResolution.Y);

    // 각 픽셀에서 가장 가까운 물까지의 거리를 저장할 배열
    TArray<float> DistanceToWater;
    DistanceToWater.Init(TNumericLimits<float>::Max(), InResolution.X * InResolution.Y);
    
    // ==========================================================
    //            Pass 1: 물 공급원 찾기 및 거리 계산
    // ==========================================================
    
    // 1-A: 모든 물 픽셀(습기 공급원)을 찾아서 큐에 넣습니다.
    TQueue<FIntPoint> Frontier; // 너비 우선 탐색(BFS)을 위한 큐
    for (int32 y = 0; y < InResolution.Y; ++y)
    {
        for (int32 x = 0; x < InResolution.X; ++x)
        {
            const int32 Index = y * InResolution.X  + x;
            const float WorldHeight = (static_cast<float>(InHeightMap[Index]) - 32768.0f); // Z스케일 문제 무시

            if (WorldHeight <= SeaLevelWorldHeight)
            {
                DistanceToWater[Index] = 0; // 물 픽셀은 거리가 0
                Frontier.Enqueue(FIntPoint(x, y));
            }
        }
    }

    // 1-B: BFS를 사용하여 모든 육지 픽셀에서 가장 가까운 물까지의 거리를 계산합니다.
    int32 dx[] = {0, 0, 1, -1};
    int32 dy[] = {1, -1, 0, 0};

    while (!Frontier.IsEmpty())
    {
        FIntPoint Current;
        Frontier.Dequeue(Current);

        for (int32 i = 0; i < 4; ++i)
        {
            int32 nx = Current.X + dx[i];
            int32 ny = Current.Y + dy[i];
            
            if (nx >= 0 && nx < InResolution.X && ny >= 0 && ny < InResolution.Y)
            {
                int32 NeighborIndex = ny * InResolution.X + nx;
                // 아직 방문하지 않은(거리가 무한대인) 이웃 픽셀이라면
                if (DistanceToWater[NeighborIndex] == TNumericLimits<float>::Max())
                {
                    DistanceToWater[NeighborIndex] = DistanceToWater[Current.Y * InResolution.X + Current.X] + 1.0f;
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
    
    for (int32 i = 0; i < InResolution.X * InResolution.Y; ++i)
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
            float HumidityFromDistance = FMath::Exp(-DistanceToWater[i] * MoistureFalloffRate);
        
            // 2-B: 온도의 영향을 적용합니다.
            float NormalizedTemp = static_cast<float>(InTempMap[i]) / 65535.0f;
            FinalHumidity = HumidityFromDistance * (1.0f - (NormalizedTemp * TemperatureInfluenceOnHumidity));
        }

        FinalHumidity = FMath::Clamp(FinalHumidity, 0.0f, 1.0f);
        
        if (RedistributionFactor > 1.f && FinalHumidity > 0.f && FinalHumidity < 1.f)
        {
            float PowX = FMath::Pow(FinalHumidity, RedistributionFactor);
            float Pow1_X = FMath::Pow(1-FinalHumidity, RedistributionFactor);
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
        float NormalizedHumidity = (HumidityMapFloat[i] - GlobalMinHumidity) / HumidityRange;
        OutHumidityMap[i] = static_cast<uint16>(NormalizedHumidity * 65535.0f);
    }

    //ExportMap(OutHumidityMap, "HumidityMap.png");
}

void UOCGMapGenerateComponent::DecideBiome(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
    const TArray<uint16>& InHumidityMap)
{
    TArray<FColor> BiomeColorMap;
    BiomeColorMap.AddUninitialized(MapResolution.X * MapResolution.Y);
    TArray<FName> BiomeMap;
    BiomeMap.AddUninitialized(MapResolution.X * MapResolution.Y);
    
    for (auto& Biome : Biomes)
    {
        Biome.Value.WeightLayer.Init(0, MapResolution.X * MapResolution.Y);
    }

    float SeaLevelHeight = MinHeight + SeaLevel * (MaxHeight - MinHeight);
    
    for (int32 y = 0; y < MapResolution.Y; ++y)
    {
        for (int32 x = 0; x < MapResolution.X; ++x)
        {
            const int32 Index = y * MapResolution.X + x;
            float Height = static_cast<float>(InHeightMap[Index]);
            Height -= 32768.f;
            float NormalizedTemp = static_cast<float>(InTempMap[y * MapResolution.X + x]) / 65535.f;
            float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y * MapResolution.X + x]) / 65535.f;
            float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            if (y == MapResolution.Y / 2 && x == MapResolution.X / 2)
                UE_LOG(LogTemp, Display, TEXT("Temp : %f, Humidity : %f"), Temp, Humidity);
            FOCGBiomeSettings* CurrentBiome = nullptr;
            if (Height <= SeaLevelHeight)
            {
                CurrentBiome = Biomes.Find(TEXT("Water"));
            }
            else
            {
                float MinDist = TNumericLimits<float>::Max();
                for (auto& Biome : Biomes)
                {
                    if (Biome.Value.BiomeName == TEXT("Water"))
                        continue;
                    float TempDiff = FMath::Abs(Biome.Value.Temperature - Temp);
                    float HumidityDiff = FMath::Abs(Biome.Value.Humidity - Humidity);
                    HumidityDiff = (MaxTemp - MinTemp) * HumidityDiff;
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
                CurrentBiome->WeightLayer[Index] = 255;
                BiomeColorMap[Index] = CurrentBiome->Color.ToFColor(true);
                BiomeMap[Index] = CurrentBiome->BiomeName;
            }
        }
    }
    MapDataUtils::ExportMap(BiomeColorMap, MapResolution, "BiomeMap.png");
    BelndBiome(BiomeMap);
}

void UOCGMapGenerateComponent::BelndBiome(const TArray<FName>& InBiomeMap)
{
     // 초기화 및 원본 맵 복사
    TMap<FName, TArray<uint8>> OriginalWeightMaps;
    for (auto& Biome : Biomes) {
        Biome.Value.WeightLayer.Init(0, MapResolution.X * MapResolution.Y); // 최종 결과를 저장할 배열 초기화
        
        TArray<uint8> InitialWeights;
        InitialWeights.Init(0, MapResolution.X * MapResolution.Y);
        OriginalWeightMaps.Add(Biome.Value.BiomeName, InitialWeights);
    }

    // 칼같이 나뉘는 초기 웨이트맵 생성 (복사본에)
    for (int32 i = 0; i < MapResolution.X * MapResolution.Y; ++i)
    {
        if (OriginalWeightMaps.Contains(InBiomeMap[i])) {
            OriginalWeightMaps[InBiomeMap[i]][i] = 255;
        }
    }
    
    // 경계 처리를 포함한 블러 필터 적용
    // 수평 블러 결과를 저장할 임시 맵
    TMap<FName, TArray<float>> HorizontalPassMaps;
    for (const auto& Elem : OriginalWeightMaps) {
        HorizontalPassMaps.Add(Elem.Key, TArray<float>());
        HorizontalPassMaps.FindChecked(Elem.Key).Init(0.f, MapResolution.X * MapResolution.Y);
    }
    
    const int32 BlendRadius = BiomeBlendRadius;

    // 수평 블러 (슬라이딩 윈도우)
    for (const auto& Elem : OriginalWeightMaps)
    {
        const FName& BiomeName = Elem.Key;
        const TArray<uint8>& OriginalLayer = Elem.Value;
        TArray<float>& HorizontalPassLayer = HorizontalPassMaps.FindChecked(BiomeName);

        for (int32 y = 0; y < MapResolution.Y; ++y)
        {
            float Sum = 0;
            // 첫 픽셀의 윈도우 합계 계산
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i) {
                int32 SampleX = FMath::Clamp(i, 0, MapResolution.Y - 1);
                Sum += OriginalLayer[y * MapResolution.X  + SampleX];
            }
            HorizontalPassLayer[y * MapResolution.X  + 0] = Sum;

            // 슬라이딩 윈도우
            for (int32 x = 1; x < MapResolution.X ; ++x) {
                int32 OldX = FMath::Clamp(x - BlendRadius - 1, 0, MapResolution.X  - 1);
                int32 NewX = FMath::Clamp(x + BlendRadius, 0, MapResolution.X  - 1);
                Sum += OriginalLayer[y * MapResolution.X  + NewX] - OriginalLayer[y * MapResolution.X + OldX];
                HorizontalPassLayer[y * MapResolution.X  + x] = Sum;
            }
        }
    }

    // 수직 블러 (슬라이딩 윈도우)
    const float BlendFactor = 1.f / ((BlendRadius * 2 + 1) * (BlendRadius * 2 + 1));
    for (const auto& Elem : HorizontalPassMaps)
    {
        const FName& BiomeName = Elem.Key;
        const TArray<float>& HorizontalPassLayer = Elem.Value;
        TArray<uint8>& FinalLayer = Biomes.Find(BiomeName)->WeightLayer;

        for (int32 x = 0; x < MapResolution.X; ++x)
        {
            float Sum = 0;
            // 첫 픽셀의 윈도우 합계 계산
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i) {
                int32 SampleY = FMath::Clamp(i, 0, MapResolution.X - 1);
                Sum += HorizontalPassLayer[SampleY * MapResolution.X  + x];
            }
            FinalLayer[x] = FMath::RoundToInt(Sum * BlendFactor);

            // 슬라이딩 윈도우
            for (int32 y = 1; y < MapResolution.Y; ++y) {
                int32 OldY = FMath::Clamp(y - BlendRadius - 1, 0, MapResolution.X - 1);
                int32 NewY = FMath::Clamp(y + BlendRadius, 0, MapResolution.X - 1);
                Sum += HorizontalPassLayer[NewY * MapResolution.X + x] - HorizontalPassLayer[OldY * MapResolution.X + x];
                FinalLayer[y * MapResolution.X + x] = FMath::RoundToInt(Sum * BlendFactor);
            }
        }
    }
    
    // 각 픽셀에 대해 모든 바이옴의 블러링된 웨이트 합이 255가 되도록 보정
    for (int32 i = 0; i < MapResolution.X * MapResolution.Y; ++i)
    {
        float TotalWeight = 0;
        for (auto& Biome : Biomes)
        {
            TotalWeight += Biome.Value.WeightLayer[i];
        }

        if (TotalWeight > 0)
        {
            float NormalizationFactor = 255.f / TotalWeight;
            for (auto& Biome : Biomes)
            {
                Biome.Value.WeightLayer[i] = FMath::RoundToInt(Biome.Value.WeightLayer[i] * NormalizationFactor);
            }
        }
    }
}

void UOCGMapGenerateComponent::ExportMap(const TArray<uint16>& InMap, const FString& FileName) const
{
#if WITH_EDITOR
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
    if (ImageWrapper->SetRaw(InMap.GetData(), InMap.GetAllocatedSize(), MapResolution.X, MapResolution.Y, ERGBFormat::Gray, 16))
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
