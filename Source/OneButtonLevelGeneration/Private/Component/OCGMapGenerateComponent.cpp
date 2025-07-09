// Copyright (c) 2025 Code1133. All rights reserved.

#include "Component/OCGMapGenerateComponent.h"

#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "OCGLevelGenerator.h"
#include "Data/MapData.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"
#include "GameFramework/CharacterMovementComponent.h"


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
    if (!MapPreset) return;

    Initialize(MapPreset);

    const FIntPoint CurMapResolution = MapPreset->MapResolution;

    //Height Map 채우기
    GenerateHeightMap(MapPreset, CurMapResolution, HeightMapData);
    //온도 맵 생성
    GenerateTempMap(MapPreset, HeightMapData, TemperatureMapData);
    //습도 맵 계산
    GenerateHumidityMap(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData);
    //높이, 온도, 습도 기반 바이옴 결정
    TArray<const FOCGBiomeSettings*> BiomeMap; 
    DecideBiome(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData, BiomeMap);
    //바이옴 별 특징 적용
    if (MapPreset->bModifyTerrainByBiome)
        ModifyLandscapeWithBiome(MapPreset, HeightMapData, BiomeMap);
    //하이트맵 부드럽게 만들기
    SmoothHeightMap(MapPreset, HeightMapData);
    //수정된 하이트맵에 따라서 물 바이옴 다시 계산
    FinalizeBiome(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData, BiomeMap);
    //침식 진행
    if (MapPreset->bErosion)
        ErosionPass(MapPreset, HeightMapData);
    GetMaxMinHeight(MapPreset, HeightMapData);
    ExportMap(MapPreset, HeightMapData, "HeightMap.png");
}

FIntPoint UOCGMapGenerateComponent::FixToNearestValidResolution(const FIntPoint InResolution)
{
    auto Fix = [](int32 Value) {
        int32 Pow = FMath::RoundToInt(FMath::Log2(static_cast<float>(Value - 1)));
        return FMath::Pow(2.f, static_cast<float>(Pow)) + 1;
    };

    return FIntPoint(Fix(InResolution.X), Fix(InResolution.Y));
}

float UOCGMapGenerateComponent::HeightMapToWorldHeight(uint16 Height)
{
    return (Height - 32768.f) * LandscapeZScale / 128.f;
}

uint16 UOCGMapGenerateComponent::WorldHeightToHeightMap(float Height)
{
    return static_cast<uint16>(Height * 128.f / LandscapeZScale + 32768.f);
}


void UOCGMapGenerateComponent::Initialize(const UMapPreset* MapPreset)
{
    Stream.Initialize(MapPreset->Seed);
    
    InitializeNoiseOffsets(MapPreset);

    if (MapPreset->bContainWater)
    {
        PlainHeight = MapPreset->SeaLevel * 1.005f;
    }
    else
    {
        PlainHeight = 0.f;
    }
    
    NoiseScale = 1;
    if (MapPreset->ApplyScaleToNoise && MapPreset->LandscapeScale > 1)
        NoiseScale = FMath::LogX(50.f, MapPreset->LandscapeScale) + 1;

    LandscapeZScale = (MapPreset->MaxHeight - MapPreset->MinHeight) * 0.001953125f;
    
    WeightLayers.Empty();
}

void UOCGMapGenerateComponent::InitializeNoiseOffsets(const UMapPreset* MapPreset)
{
    PlainNoiseOffset.X = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
    PlainNoiseOffset.Y = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);

    MountainNoiseOffset.X = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
    MountainNoiseOffset.Y = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
    
    BlendNoiseOffset.X = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
    BlendNoiseOffset.Y = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);

    DetailNoiseOffset.X = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
    DetailNoiseOffset.Y = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);

    IslandNoiseOffset.X = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
    IslandNoiseOffset.Y = Stream.FRandRange(-MapPreset->StandardNoiseOffset, MapPreset->StandardNoiseOffset);
}

void UOCGMapGenerateComponent::GenerateHeightMap(const UMapPreset* MapPreset, const FIntPoint CurMapResolution, TArray<uint16>& OutHeightMap)
{
    HeightMapData.SetNumUninitialized(CurMapResolution.X * CurMapResolution.Y);
    
    // 2. 하이트맵 데이터 채우기
    for (int32 y = 0; y < CurMapResolution.Y; ++y)
    {
        for (int32 x = 0; x <CurMapResolution.X; ++x)
        {
            const float CalculatedHeight = CalculateHeightForCoordinate(MapPreset, x, y);
            const float NormalizedHeight = CalculatedHeight * 65535.f;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            HeightMapData[y * CurMapResolution.X + x] = HeightValue;
        }
    }
}

float UOCGMapGenerateComponent:: CalculateHeightForCoordinate(const UMapPreset* MapPreset, const int32 InX, const int32 InY) const
{
	const float HeightRange = MapPreset->MaxHeight - MapPreset->MinHeight;
	// ==========================================================
    //            1. 낮은 주파수로 거대한 산맥 생성
    // ==========================================================
    float MountainNoiseX = InX * MapPreset->ContinentNoiseScale * NoiseScale + MountainNoiseOffset.X;
    float MountainNoiseY = InY * MapPreset->ContinentNoiseScale * NoiseScale + MountainNoiseOffset.Y;
    //-1~1 범위
    float MountainHeight = FMath::PerlinNoise2D(FVector2D(MountainNoiseX, MountainNoiseY));
    MountainHeight *= 2.f;
    MountainHeight = FMath::Clamp(MountainHeight, -1.f, 1.f);

    // ==========================================================
    //            2. 산맥 지형 디테일 추가
    // ==========================================================
    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float TerrainNoise = 0.0f;
    float MaxPossibleAmplitude = 0.0f;
    
    for (int32 i = 0; i < MapPreset->Octaves; ++i)
    {
        float NoiseInputX = (InX * MapPreset->TerrainNoiseScale * NoiseScale * Frequency) + DetailNoiseOffset.X;
        float NoiseInputY = (InY * MapPreset->TerrainNoiseScale * NoiseScale * Frequency) + DetailNoiseOffset.Y;
        float PerlinValue = FMath::PerlinNoise2D(FVector2D(NoiseInputX, NoiseInputY)); // -1 ~ 1
        TerrainNoise += PerlinValue * Amplitude;
        MaxPossibleAmplitude += Amplitude;
        Amplitude *= MapPreset->Persistence;
        Frequency *= MapPreset->Lacunarity;
    }
    // 지형 노이즈를 -1 ~ 1 사이로 정규화
    TerrainNoise /= MaxPossibleAmplitude;
    TerrainNoise *= 0.3f;
    MountainHeight += TerrainNoise;
    
    // ==========================================================
    //            3. 평야와 산맥을 결합할 블렌드 마스크 생성
    // ==========================================================
    float BlendNoiseX = InX * MapPreset->ContinentNoiseScale * NoiseScale + BlendNoiseOffset.X;
    float BlendNoiseY = InY * MapPreset->ContinentNoiseScale * NoiseScale + BlendNoiseOffset.Y;
    //0~1 범위
    float BlendNoise = FMath::PerlinNoise2D(FVector2D(BlendNoiseX, BlendNoiseY)) * 0.5f + 0.5f;
    //분포를 양쪽 끝으로 확산
    if (MapPreset->RedistributionFactor > 1.f && BlendNoise > 0.f && BlendNoise < 1.f)
    {
        float PowX = FMath::Pow(BlendNoise, MapPreset->RedistributionFactor);
        float Pow1_X = FMath::Pow(1-BlendNoise, MapPreset->RedistributionFactor);
        BlendNoise = PowX/(PowX + Pow1_X);
    }

    // ==========================================================
    //            4. 블렌드 마스크에 따라 최종 높이 결정
    // ==========================================================
    MountainHeight = MountainHeight * 0.5f + 0.5f;
    float Height = FMath::Lerp(PlainHeight, MountainHeight, BlendNoise);

    // ==========================================================
    //            5. 섬 모양 적용
    // ==========================================================
    if (MapPreset->bIsland)
    {
        //중앙에서의 거리 계산
        float nx = (static_cast<float>(InX) / MapPreset->MapResolution.X) * 2.f - 1.f;
        float ny = (static_cast<float>(InY) / MapPreset->MapResolution.Y) * 2.f - 1.f;
        float Distance = FMath::Sqrt(nx * nx + ny * ny);
        //해안선 노이즈 생성
        float IslandNoiseX = InX * MapPreset->IslandShapeNoiseScale * NoiseScale + IslandNoiseOffset.X;
        float IslandNoiseY = InY * MapPreset->IslandShapeNoiseScale * NoiseScale + IslandNoiseOffset.Y;
        //-1~1 범위
        float IslandNoise = FMath::PerlinNoise2D(FVector2D(IslandNoiseX, IslandNoiseY));
        //거리 왜곡
        float DistortedDistance = Distance + IslandNoise * MapPreset->IslandShapeNoiseStrength;
        //최종 섬 마스크 생성
        float IslandMask = 1.f - DistortedDistance;
        IslandMask *= 3.f;
        IslandMask = FMath::Clamp(IslandMask, 0.f, 1.f);
        IslandMask = FMath::Pow(IslandMask, MapPreset->IslandFalloffExponent);
        IslandMask = FMath::Clamp(IslandMask, 0.f, 1.f);
        Height *= IslandMask;
        Height = FMath::Clamp(Height, 0.f, 1.f);
    }

    return Height;
}

void UOCGMapGenerateComponent::ErosionPass(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap)
{
    if (MapPreset->NumErosionIterations <= 0) return;

    // 1. 침식 브러시 인덱스 미리 계산 (최적화)
    InitializeErosionBrush();
    
    // 2. 계산을 위해 uint16 맵을 float 맵으로 변환
    TArray<float> HeightMapFloat;
    HeightMapFloat.SetNumUninitialized(InOutHeightMap.Num());
    for (int i = 0; i < InOutHeightMap.Num(); ++i)
    {
        HeightMapFloat[i] = HeightMapToWorldHeight(InOutHeightMap[i]);
    }

    float SeaLevelHeight;
    if (MapPreset->bContainWater)
    {
        SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
    }
    else
    {
        SeaLevelHeight = MapPreset->MinHeight;
    }

    float LandscapeScale = MapPreset->LandscapeScale * 100.f;

    // 3. 메인 시뮬레이션 루프
    for (int i = 0; i < MapPreset->NumErosionIterations; ++i)
    {
        // --- 물방울 초기화 ---
        float PosX = Stream.RandRange(1.f, MapPreset->MapResolution.X - 2.f);
        float PosY = Stream.RandRange(1.f, MapPreset->MapResolution.Y - 2.f);
        float DirX = 0, DirY = 0;
        float Speed = MapPreset->InitialSpeed;
        float Water = MapPreset->InitialWaterVolume;
        float Sediment = 0;

        // --- 한 물방울의 생명주기 루프 ---
        for (int Lifetime = 0; Lifetime <MapPreset-> MaxDropletLifetime; ++Lifetime)
        {
            int32 NodeX = static_cast<int32>(PosX);
            int32 NodeY = static_cast<int32>(PosY);
            int32 DropletIndex = NodeY * MapPreset->MapResolution.X + NodeX;

            // 맵 경계를 벗어나면 종료
            if (NodeX < 0 || NodeX >= MapPreset->MapResolution.X - 1 || NodeY < 0 || NodeY >= MapPreset->MapResolution.Y - 1)
            {
                break;
            }

            // 현재 위치의 높이와 경사 계산
            FVector2D Gradient;
            float CurrentHeight = CalculateHeightAndGradient(MapPreset, HeightMapFloat, LandscapeScale, PosX, PosY, Gradient);
            
            // 관성을 적용하여 새로운 방향 계산
            DirX = (DirX * MapPreset->DropletInertia) - (Gradient.X * (1 - MapPreset->DropletInertia));
            DirY = (DirY * MapPreset->DropletInertia) - (Gradient.Y * (1 - MapPreset->DropletInertia));
            
            // 방향 벡터 정규화
            float Len = FMath::Sqrt(DirX * DirX + DirY * DirY);
            if (Len > KINDA_SMALL_NUMBER)
            {
                DirX /= Len;
                DirY /= Len;
            }
            
            // 새로운 위치로 이동
            PosX += DirX;
            PosY += DirY;
            
            if (PosX <= 0 || PosX >= MapPreset->MapResolution.X - 1 ||
                PosY <= 0 || PosY >= MapPreset->MapResolution.Y - 1)
            {
                break;
            }
            
            // 이동 후 높이와 높이 차이 계산
            float NewHeight = CalculateHeightAndGradient(MapPreset, HeightMapFloat, LandscapeScale, PosX, PosY, Gradient);
            if (NewHeight <= SeaLevelHeight)
                break;
            float HeightDifference = NewHeight - CurrentHeight;
            
            // 흙 운반 용량 계산
            float SedimentCapacity = FMath::Max(-HeightDifference * Speed * Water * MapPreset->SedimentCapacityFactor, MapPreset->MinSedimentCapacity);
            
            // 침식 또는 퇴적
            if (Sediment > SedimentCapacity || HeightDifference > 0)
            {
                // 퇴적
                float AmountToDeposit = (HeightDifference > 0) ? FMath::Min(Sediment, HeightDifference) : (Sediment - SedimentCapacity) * MapPreset->DepositSpeed;
                Sediment -= AmountToDeposit;
                
                // 미리 계산된 브러시를 사용하여 주변에 흙을 쌓음
                const TArray<int32>& Indices = ErosionBrushIndices[DropletIndex];
                const TArray<float>& Weights = ErosionBrushWeights[DropletIndex];
                for (int j = 0; j < Indices.Num(); j++)
                {
                    HeightMapFloat[Indices[j]] += AmountToDeposit * Weights[j];
                }
            }
            else
            {
                // 침식
                float AmountToErode = FMath::Min((SedimentCapacity - Sediment), -HeightDifference) * MapPreset->ErodeSpeed;
                
                // 미리 계산된 브러시를 사용하여 주변 땅을 깎음
                const TArray<int32>& Indices = ErosionBrushIndices[DropletIndex];
                const TArray<float>& Weights = ErosionBrushWeights[DropletIndex];
                for (int j = 0; j < Indices.Num(); j++)
                {
                    HeightMapFloat[Indices[j]] -= AmountToErode * Weights[j];
                }
                Sediment += AmountToErode;
            }
            
            // 속도 및 물 업데이트
            Speed = FMath::Sqrt(FMath::Max(0.f, Speed * Speed - HeightDifference * MapPreset->Gravity));
            Water *= (1.0f - MapPreset->EvaporateSpeed);
        }
    }
    uint16 SeaHeight = WorldHeightToHeightMap(SeaLevelHeight);
    // 4. float 맵을 다시 uint16 맵으로 변환
    for (int i = 0; i < HeightMapFloat.Num(); ++i)
    {
        //퇴적으로 인해 기존 높이 보다 높게 흙이 쌓인 걸 제거
        uint16 Height =  WorldHeightToHeightMap(HeightMapFloat[i]);
        uint16 NewHeight = FMath::Min(Height, InOutHeightMap[i]);
        //물 높이 이하로 내려가는 걸 방지
        if (InOutHeightMap[i] >= SeaHeight)
            NewHeight = FMath::Max(NewHeight, SeaHeight);
        InOutHeightMap[i] = FMath::Clamp(NewHeight, 0, 65535);
    }
}

void UOCGMapGenerateComponent::InitializeErosionBrush()
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    // 이미 현재 반경으로 초기화했다면 다시 하지 않음
    int NewSize = MapPreset->MapResolution.X * MapPreset->MapResolution.Y;
    if (CurrentErosionRadius == MapPreset->ErosionRadius && ErosionBrushIndices.Num() == NewSize)
    {
        return;
    }

    ErosionBrushIndices.SetNum(MapPreset->MapResolution.X * MapPreset->MapResolution.Y);
    ErosionBrushWeights.SetNum(MapPreset->MapResolution.X * MapPreset->MapResolution.Y);

    for (int32 i = 0; i < MapPreset->MapResolution.X * MapPreset->MapResolution.Y; i++)
    {
        int32 CenterX = i % MapPreset->MapResolution.X;
        int32 CenterY = i / MapPreset->MapResolution.Y;

        float WeightSum = 0;
        TArray<int32>& Indices = ErosionBrushIndices[i];
        TArray<float>& Weights = ErosionBrushWeights[i];

        for (int32 y = -MapPreset->ErosionRadius; y <= MapPreset->ErosionRadius; y++)
        {
            for (int32 x = -MapPreset->ErosionRadius; x <= MapPreset->ErosionRadius; x++)
            {
                float DistSquared = x * x + y * y;
                float Dist = FMath::Sqrt(DistSquared);
                if (Dist <= MapPreset->ErosionRadius)
                {
                    int32 CoordX = CenterX + x;
                    int32 CoordY = CenterY + y;
                    if (CoordX >= 0 && CoordX < MapPreset->MapResolution.X && CoordY >= 0 && CoordY < MapPreset->MapResolution.Y)
                    {
                        float Weight = 1.0f - (Dist / MapPreset->ErosionRadius);
                        WeightSum += Weight;
                        Indices.Add(CoordY * MapPreset->MapResolution.X + CoordX);
                        Weights.Add(Weight);
                    }
                }
            }
        }
        
        // 가중치 정규화 (총합이 1이 되도록)
        if (WeightSum > 0)
        {
            for (int32 j = 0; j < Weights.Num(); j++)
            {
                Weights[j] /= WeightSum;
            }
        }
    }
    CurrentErosionRadius = MapPreset->ErosionRadius;
}

float UOCGMapGenerateComponent::CalculateHeightAndGradient(const UMapPreset* MapPreset, const TArray<float>& HeightMap, const float LandscapeScale, 
    float PosX, float PosY,FVector2D& OutGradient)
{
    int32 CoordX = static_cast<int32>(PosX);
    int32 CoordY = static_cast<int32>(PosY);

    // 보간을 위한 가중치 계산
    float x = PosX - CoordX;
    float y = PosY - CoordY;

    // 4개의 주변 셀 인덱스
    int32 Index_00 = CoordY * MapPreset->MapResolution.X + CoordX;
    int32 Index_10 = Index_00 + 1;
    int32 Index_01 = Index_00 + MapPreset->MapResolution.X;
    int32 Index_11 = Index_01 + 1;

    // 4개 셀의 높이
    float Height_00 = HeightMap[Index_00];
    float Height_10 = HeightMap[Index_10];
    float Height_01 = HeightMap[Index_01];
    float Height_11 = HeightMap[Index_11];

    // 경사(Gradient) 계산
    OutGradient.X = (Height_10 - Height_00) * (1 - y) + (Height_11 - Height_01) * y / LandscapeScale;
    OutGradient.Y = (Height_01 - Height_00) * (1 - x) + (Height_11 - Height_10) * x / LandscapeScale;

    // 높이(Bilinear Interpolation) 계산
    return Height_00 * (1 - x) * (1 - y) + Height_10 * x * (1 - y) + Height_01 * (1 - x) * y + Height_11 * x * y;
}

void UOCGMapGenerateComponent::ModifyLandscapeWithBiome(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap)
{
    TArray<float> MinHeights;
    TArray<float> BlurredMinHeights;
    //각 바이옴 구역의 최소 높이를 월드 기준 float로 반환
    CalculateBiomeMinHeights(InOutHeightMap, InBiomeMap, MinHeights, MapPreset);
    if (MapPreset->BiomeHeightBlendRadius > 0)
        BlurBiomeMinHeights(BlurredMinHeights, MinHeights, MapPreset);
    else
        BlurredMinHeights = MinHeights;
    float SeaLevel;
    if (MapPreset->bContainWater)
        SeaLevel = MapPreset->SeaLevel;
    else
        SeaLevel = 0.f;
    float SeaLevelHeightF = SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight) + MapPreset->MinHeight;
    uint16 SeaLevelHeight = WorldHeightToHeightMap(SeaLevelHeightF);
    for (int32 y = 0; y<MapPreset->MapResolution.Y; y++)
    {
        for (int32 x = 0; x<MapPreset->MapResolution.X; x++)
        {
            int32 Index = y * MapPreset->MapResolution.X + x;
            const FOCGBiomeSettings* CurrentBiome = InBiomeMap[Index];
            if (!CurrentBiome || CurrentBiome->BiomeName == TEXT("Water"))
                continue;
            uint16 CurrentHeight = InOutHeightMap[Index];
            float MtoPRatio = 0;
            for (int i=1; i<WeightLayers.Num(); i++)
            {
                FString LayerNameStr = FString::Printf(TEXT("Layer%d"), i);
                FName LayerName(LayerNameStr);
                float CurrentBiomeWeight = WeightLayers[LayerName][Index] / 255.f;
                if (CurrentBiomeWeight <= 0.f)
                    continue;
                MtoPRatio+=MapPreset->Biomes[i - 1].MountainRatio * CurrentBiomeWeight;
            }
            uint16 BiomeMinHeight = WorldHeightToHeightMap(BlurredMinHeights[Index]);
            uint16 TargetPlainHeight = FMath::Lerp(CurrentHeight, BiomeMinHeight, (1.0f - MtoPRatio) * MapPreset->PlainSmoothFactor);
            
            float DetailNoise = FMath::PerlinNoise2D(FVector2D(static_cast<float>(x), static_cast<float>(y)) *
                MapPreset->BiomeNoiseScale) * MapPreset->BiomeNoiseAmplitude + MapPreset->BiomeNoiseAmplitude;
            float MountainHeight = DetailNoise * (MapPreset->MaxHeight - MapPreset->MinHeight) * 128.f/LandscapeZScale;

            uint16 NewHeight = FMath::Lerp(TargetPlainHeight, TargetPlainHeight + MountainHeight, MtoPRatio);
            NewHeight = FMath::Max(NewHeight, SeaLevelHeight);
            InOutHeightMap[Index] = NewHeight;
        }
    }
}

void UOCGMapGenerateComponent::CalculateBiomeMinHeights(const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap,
    TArray<float>& OutMinHeights, const UMapPreset* MapPreset)
{
    FIntPoint MapSize = MapPreset->MapResolution;
    const int32 TotalPixels = MapSize.X * MapSize.Y;
    TArray<int32> RegionIDMap;
    RegionIDMap.Init(0, TotalPixels);
    OutMinHeights.Init(0, TotalPixels);

    TMap<int32, float> RegionMinHeight;

    int32 CurrentRegionID = 1;

    for (int32 y = 0; y<MapSize.Y; y++)
    {
        for (int32 x = 0; x<MapSize.X; x++)
        {
            if (RegionIDMap[y*MapSize.X + x] == 0)
            {
                float MinimumHeight;
                GetBiomeStats(MapSize, x, y, CurrentRegionID, MinimumHeight, RegionIDMap, InHeightMap, InBiomeMap);

                RegionMinHeight.Add(CurrentRegionID, MinimumHeight);
                CurrentRegionID++;
            }
        }
    }
    
    for (int32 i=0; i<TotalPixels; i++)
    {
        int32 RegionID = RegionIDMap[i];
        OutMinHeights[i] = RegionMinHeight.FindRef(RegionID);
    }
}

void UOCGMapGenerateComponent::BlurBiomeMinHeights(TArray<float>& OutMinHeights, const TArray<float>& InMinHeights, const UMapPreset* MapPreset)
{
    float BlendRadius = MapPreset->BiomeHeightBlendRadius;
    FIntPoint MapSize = MapPreset->MapResolution;
    int32 TotalPixels = MapSize.X * MapSize.Y;
    OutMinHeights.SetNumUninitialized(TotalPixels);
    float SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
    //Horizontal Pass
    TArray<float> HorizontalPass;
    HorizontalPass.Init(0, TotalPixels);
    for (int32 y = 0; y < MapSize.Y; ++y)
    {
        float Sum = 0;
        int32 ValidPixelCount = 0;
        //첫 픽셀 값 계산
        for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
        {
            const int32 CurrentX = FMath::Clamp(i, 0, MapSize.X - 1);
            const int32 Index = y * MapSize.X + CurrentX;
            Sum+=InMinHeights[Index];
            ValidPixelCount++;
        }
        HorizontalPass[y*MapSize.X + 0] = (ValidPixelCount > 0) ? Sum / ValidPixelCount : InMinHeights[y*MapSize.X + 0];
        // 슬라이딩 윈도우
        for (int32 x = 1; x < MapSize.X ; ++x)
        {
            const int32 OldX = FMath::Clamp(x - BlendRadius - 1, 0, MapSize.X  - 1);
            const int32 OldIndex = y * MapSize.X + OldX;
            Sum-=InMinHeights[OldIndex];
            ValidPixelCount--;
            const int32 NewX = FMath::Clamp(x + BlendRadius, 0, MapSize.X  - 1);
            const int32 NewIndex = y * MapSize.X + NewX;
            Sum+=InMinHeights[NewIndex];
            ValidPixelCount++;
            HorizontalPass[y * MapSize.X  + x] = (ValidPixelCount > 0) ? Sum / ValidPixelCount : InMinHeights[y * MapSize.X + x];
        }
    }
    //Vertical Pass
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        float Sum = 0;
        int32 ValidPixelCount = 0;
        //첫 픽셀 값 계산
        for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
        {
            const int32 CurrentY = FMath::Clamp(i, 0, MapSize.Y - 1);
            const int32 Index = CurrentY * MapSize.X + x;
            Sum+=HorizontalPass[Index];
            ValidPixelCount++;
        }
        OutMinHeights[0 * MapSize.X + x] = (ValidPixelCount > 0) ? Sum / ValidPixelCount : HorizontalPass[0 * MapSize.X + x];
        // 슬라이딩 윈도우
        for (int32 y = 1; y < MapSize.Y ; ++y)
        {
            const int32 OldY = FMath::Clamp(y - BlendRadius - 1, 0, MapSize.Y  - 1);
            const int32 OldIndex = OldY * MapSize.X + x;
            Sum-=HorizontalPass[OldIndex];
            ValidPixelCount--;
            const int32 NewY = FMath::Clamp(y + BlendRadius, 0, MapSize.Y  - 1);
            const int32 NewIndex = NewY * MapSize.X + x;
            Sum+=HorizontalPass[NewIndex];
            ValidPixelCount++;
            OutMinHeights[y * MapSize.X  + x] = (ValidPixelCount > 0) ? Sum / ValidPixelCount : HorizontalPass[y * MapSize.X + x];
        }
    }
}

void UOCGMapGenerateComponent::GetBiomeStats(FIntPoint MapSize, int32 x, int32 y, int32 RegionID, float& OutMinHeight,
    TArray<int32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap)
{
    TQueue<FIntPoint> Queue;
    Queue.Enqueue(FIntPoint(x, y));

    const FOCGBiomeSettings* TargetBiome = InBiomeMap[y*MapSize.X + x];
    RegionIDMap[y*MapSize.X + x] = RegionID;

    OutMinHeight = FLT_MAX;

    FIntPoint CurrentPoint;
    while (Queue.Dequeue(CurrentPoint))
    {
        uint32 CurrentIndex = CurrentPoint.Y * MapSize.X + CurrentPoint.X;
        float CurrentHeight = HeightMapToWorldHeight(InHeightMap[CurrentIndex]);
        if (CurrentHeight < OutMinHeight)
            OutMinHeight = CurrentHeight;

        const FIntPoint Neighbors[] =
        {
            FIntPoint(CurrentPoint.X + 1, CurrentPoint.Y), FIntPoint(CurrentPoint.X - 1, CurrentPoint.Y),
            FIntPoint(CurrentPoint.X, CurrentPoint.Y + 1), FIntPoint(CurrentPoint.X, CurrentPoint.Y - 1),
        };

        for (const FIntPoint& Neighbor : Neighbors)
        {
            if (Neighbor.X >= 0 && Neighbor.X < MapSize.X && Neighbor.Y >= 0 && Neighbor.Y < MapSize.Y)
            {
                int32 NeighborIndex = Neighbor.Y * MapSize.X + Neighbor.X;
                if (RegionIDMap[NeighborIndex] == 0 && InBiomeMap[NeighborIndex] == TargetBiome)
                {
                    RegionIDMap[NeighborIndex] = RegionID;
                    Queue.Enqueue(Neighbor);
                }
            }
        }
    }
}

void UOCGMapGenerateComponent::GetMaxMinHeight(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap)
{
    TArray<float> HeightMapFloat;
    int32 TotalPixel = MapPreset->MapResolution.X * MapPreset->MapResolution.Y;
    HeightMapFloat.SetNumUninitialized(TotalPixel);
    float Max = MapPreset->MinHeight;
    float Min = MapPreset->MaxHeight;
    for (int32 i=0; i < TotalPixel; i++)
    {
        HeightMapFloat[i] = HeightMapToWorldHeight(InHeightMap[i]);
        if (HeightMapFloat[i] > Max)
            Max = HeightMapFloat[i];
        if (HeightMapFloat[i] < Min)
            Min = HeightMapFloat[i];
    }
    MaxHeight = Max;
    MinHeight = Min;
}

void UOCGMapGenerateComponent::SmoothHeightMap(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap)
{
    if (!MapPreset->bSmoothHeight)
        return;
    
    FIntPoint MapSize = MapPreset->MapResolution;
    
    const float SpikeThreshold = (FMath::Tan(FMath::DegreesToRadians(MapPreset->MaxSlopeAngle)) *
        100.f * MapPreset->LandscapeScale) * 128.f / LandscapeZScale;

    const float DiagonalSpikeThreshold = SpikeThreshold * 1.41421f;

    TArray<FIntPoint> Neighbors =
    {
        {0, -1}, {1,-1}, {1, 0}, {1, 1},
        {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}
    };

    int32 SpikeCount = 0;
    
    for (int Iteration=0; Iteration<MapPreset->SmoothingIteration; Iteration++)
    {
        SpikeCount = 0;
        for (int32 y=1; y<MapSize.Y-1; y++)
        {
            for (int32 x=1; x<MapSize.X-1; x++)
            {
                const int32 Index = y*MapSize.X + x;
                float CenterHeight = InOutHeightMap[Index];

                float TotalHeightToMove = 0;
                
                for (int i=0; i < Neighbors.Num(); i++)
                {
                    const int32 NeighborIndex = (Neighbors[i].Y + y) * MapSize.X + (Neighbors[i].X + x);
                    const float NeighborHeight = InOutHeightMap[NeighborIndex];
                    const float HeightDiff = CenterHeight - NeighborHeight;

                    const bool bIsDiagonal = (Neighbors[i].X != 0 && Neighbors[i].Y != 0);
                    const float CurrentThreshold = bIsDiagonal ? DiagonalSpikeThreshold : SpikeThreshold;
                    
                    if (HeightDiff > CurrentThreshold)
                    {
                        float HeightToMove = HeightDiff - CurrentThreshold;
                        uint16 NewHeight = InOutHeightMap[NeighborIndex] + HeightToMove;
                        TotalHeightToMove += HeightToMove;
                        InOutHeightMap[NeighborIndex] = FMath::Clamp(NewHeight, 0, 65535);
                    }
                }

                if (TotalHeightToMove > 0)
                {
                    TotalHeightToMove = FMath::Min(TotalHeightToMove, SpikeThreshold);
                    uint16 NewHeight = InOutHeightMap[Index] - TotalHeightToMove;
                    InOutHeightMap[Index] = FMath::Clamp(NewHeight, 0, 65535);
                }
            }
        }
        UE_LOG(LogTemp, Display, TEXT("SpikeCount: %d, Iteration: %d"), SpikeCount, Iteration);
    }
}

void UOCGMapGenerateComponent::GenerateTempMap(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap)
{
    const FIntPoint CurResolution = MapPreset->MapResolution;
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
            //       1. 노이즈 기반으로 기본 온도 설정
            // ==========================================================
            // 매우 낮은 주파수의 노이즈를 사용하여 따뜻한 지역과 추운 지역의 큰 덩어리를 만듭니다.
            float TempNoiseInputX = x * MapPreset->TemperatureNoiseScale + PlainNoiseOffset.X;
            float TempNoiseInputY = y * MapPreset->TemperatureNoiseScale + PlainNoiseOffset.Y;
            
            float TempNoiseAlpha = FMath::PerlinNoise2D(FVector2D(TempNoiseInputX, TempNoiseInputY)) * 0.5f + 0.5f;

            // 노이즈 값에 따라 MinTemp와 MaxTemp 사이의 기본 온도를 결정합니다.
            float BaseTemp = FMath::Lerp(MapPreset->MinTemp, MapPreset->MaxTemp, TempNoiseAlpha);

            // ==========================================================
            //        2. 고도에 따른 온도 감쇠
            // ==========================================================
            const float WorldHeight = HeightMapToWorldHeight(InHeightMap[Index]);
            float SeaLevelHeight;
            if (MapPreset->bContainWater)
            {
                SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
            }
            else
            {
                SeaLevelHeight = MapPreset->MinHeight;
            }
            
            if (WorldHeight > SeaLevelHeight)
            {
                BaseTemp -= ((WorldHeight - SeaLevelHeight) / 1000.0f) * MapPreset->TempDropPer1000Units;
            }

            float NormalizedBaseTemp = (BaseTemp - MapPreset->MinTemp) / TempRange;
            //온도 차이 벌리기
            //if (MapPreset->RedistributionFactor > 1.f && NormalizedBaseTemp > 0.f && NormalizedBaseTemp < 1.f)
            //{
            //    const float PowX = FMath::Pow(NormalizedBaseTemp, MapPreset->RedistributionFactor);
            //    const float Pow1_X = FMath::Pow(1-NormalizedBaseTemp, MapPreset->RedistributionFactor);
            //    NormalizedBaseTemp = PowX/(PowX + Pow1_X);
            //}
            BaseTemp = MapPreset->MinTemp + NormalizedBaseTemp * TempRange;
            // ==========================================================
            //          3. 최종 온도 값 저장
            // ==========================================================
            const float FinalTemp = FMath::Clamp(BaseTemp, MapPreset->MinTemp, MapPreset->MaxTemp);
            
            TempMapFloat[Index] = FinalTemp;
            
            if (FinalTemp < GlobalMinTemp) GlobalMinTemp = FinalTemp;
            if (FinalTemp > GlobalMaxTemp) GlobalMaxTemp = FinalTemp;
        }
    }

    CachedGlobalMinTemp = GlobalMinTemp;
    CachedGlobalMaxTemp = GlobalMaxTemp;

    // ==========================================================
    //        4. float 온도맵을 uint16 이미지 데이터로 변환
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
    ExportMap(MapPreset, OutTempMap, "TempMap.png");
}

void UOCGMapGenerateComponent::GenerateHumidityMap(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
	TArray<uint16>& OutHumidityMap)
{
    const FIntPoint CurResolution = MapPreset->MapResolution;
    if (OutHumidityMap.Num() != CurResolution.X * CurResolution.Y)
    {
        OutHumidityMap.SetNumUninitialized(CurResolution.X * CurResolution.Y);
    }
    
    float SeaLevelWorldHeight;
    if (MapPreset->bContainWater)
    {
        SeaLevelWorldHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
    }
    else
    {
        SeaLevelWorldHeight = MapPreset->MinHeight;
    }

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
            const float WorldHeight = HeightMapToWorldHeight(InHeightMap[Index]);

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
        //습도 차이 벌리기
        //if (MapPreset->RedistributionFactor > 1.f && FinalHumidity > 0.f && FinalHumidity < 1.f)
        //{
        //    const float PowX = FMath::Pow(FinalHumidity, MapPreset->RedistributionFactor);
        //    const float Pow1_X = FMath::Pow(1-FinalHumidity, MapPreset->RedistributionFactor);
        //    FinalHumidity = PowX/(PowX + Pow1_X);
        //}
        
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

    ExportMap(MapPreset, OutHumidityMap, "HumidityMap.png");
}

void UOCGMapGenerateComponent::DecideBiome(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
    const TArray<uint16>& InHumidityMap, TArray<const FOCGBiomeSettings*>& OutBiomeMap)
{
    const FIntPoint CurResolution = MapPreset->MapResolution;
    
    BiomeColorMap.SetNumUninitialized(CurResolution.X * CurResolution.Y);
    BiomeNameMap.SetNumUninitialized(CurResolution.X * CurResolution.Y);
    OutBiomeMap.SetNumUninitialized(CurResolution.X * CurResolution.Y);

    for (int Index = 1; Index <= MapPreset->Biomes.Num(); ++Index)
    {
        TArray<uint8> WeightLayer;
        WeightLayer.SetNumZeroed(CurResolution.X * CurResolution.Y);
        FString LayerNameStr = FString::Printf(TEXT("Layer%d"), Index);
        FName LayerName(LayerNameStr);
        WeightLayers.Add(LayerName, WeightLayer);
    }
    //물 따로 처리
    TArray<uint8> WaterWeightLayer;
    WaterWeightLayer.SetNumZeroed(CurResolution.X * CurResolution.Y);
    FString WaterLayerNameStr = FString::Printf(TEXT("Layer%d"), 0);
    FName WaterLayerName(WaterLayerNameStr);
    WeightLayers.Add(WaterLayerName, WaterWeightLayer);

    float SeaLevelHeight;
    if (MapPreset->bContainWater)
    {
        SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
    }
    else
    {
        SeaLevelHeight = MapPreset->MinHeight;
    }
    
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X + x;
            float Height = HeightMapToWorldHeight(InHeightMap[Index]);
            float NormalizedTemp = static_cast<float>(InTempMap[y * CurResolution.X + x]) / 65535.f;
            const float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y * CurResolution.X + x]) / 65535.f;
            const float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            if (y == CurResolution.Y / 2 && x == CurResolution.X / 2)
                UE_LOG(LogTemp, Display, TEXT("Temp : %f, Humidity : %f"), Temp, Humidity);
            const FOCGBiomeSettings* CurrentBiome = nullptr;
            const FOCGBiomeSettings* WaterBiome = &MapPreset->WaterBiome;
            uint32 CurrentBiomeIndex = INDEX_NONE;
            if (WaterBiome && Height < SeaLevelHeight)
            {
                CurrentBiome = WaterBiome;
                //물 바이옴은 첫번째 레이어
                CurrentBiomeIndex = 0;
            }
            else
            {
                float MinDist = TNumericLimits<float>::Max();
                float TempRange = MapPreset->MaxTemp - MapPreset->MinTemp;
                
                for (int32 BiomeIndex = 1; BiomeIndex <= MapPreset->Biomes.Num(); ++BiomeIndex)
                {
                    const FOCGBiomeSettings* BiomeSettings = &MapPreset->Biomes[BiomeIndex - 1];
                    float TempDiff = FMath::Abs(BiomeSettings->Temperature - Temp) / TempRange;
                    float HumidityDiff = FMath::Abs(BiomeSettings->Humidity - Humidity);
                    float Dist = FVector2D(TempDiff, HumidityDiff).Length();
                    if (Dist < MinDist)
                    {
                        MinDist = Dist;
                        CurrentBiome = BiomeSettings;
                        CurrentBiomeIndex = BiomeIndex;
                    }
                }
            }
            
            if(CurrentBiome)
            {
                FName LayerName;
                if (CurrentBiomeIndex != INDEX_NONE)
                {
                    FString LayerNameStr = FString::Printf(TEXT("Layer%d"), CurrentBiomeIndex);
                    LayerName = FName(LayerNameStr);
                    WeightLayers[LayerName][Index] = 255;
                    BiomeNameMap[Index] = LayerName;
                    OutBiomeMap[Index] = CurrentBiome;
                    BiomeColorMap[Index] = CurrentBiome->Color.ToFColor(true);
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("Current Biome index is invalid"));
                }
            }
        }
    }
    BlendBiome(MapPreset);
}

void UOCGMapGenerateComponent::FinalizeBiome(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
    const TArray<uint16>& InHumidityMap, TArray<const FOCGBiomeSettings*>& OutBiomeMap)
{
    if (!MapPreset->bContainWater)
        return;
    const FIntPoint CurResolution = MapPreset->MapResolution;

    float SeaLevelHeight = MapPreset->MinHeight + MapPreset->SeaLevel * (MapPreset->MaxHeight - MapPreset->MinHeight);
    
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X + x;
            float Height = HeightMapToWorldHeight(InHeightMap[Index]);
            if (BiomeNameMap[Index] != TEXT("Layer0") || Height < SeaLevelHeight)
                continue;
            float NormalizedTemp = static_cast<float>(InTempMap[y * CurResolution.X + x]) / 65535.f;
            const float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y * CurResolution.X + x]) / 65535.f;
            const float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            if (y == CurResolution.Y / 2 && x == CurResolution.X / 2)
                UE_LOG(LogTemp, Display, TEXT("Temp : %f, Humidity : %f"), Temp, Humidity);
            const FOCGBiomeSettings* CurrentBiome = nullptr;
            const FOCGBiomeSettings* WaterBiome = &MapPreset->WaterBiome;
            uint32 CurrentBiomeIndex = INDEX_NONE;
            if (WaterBiome && Height < SeaLevelHeight)
            {
                CurrentBiome = WaterBiome;
                //물 바이옴은 첫번째 레이어
                CurrentBiomeIndex = 0;
            }
            else
            {
                float MinDist = TNumericLimits<float>::Max();
                float TempRange = MapPreset->MaxTemp - MapPreset->MinTemp;
                
                for (int32 BiomeIndex = 1; BiomeIndex <= MapPreset->Biomes.Num(); ++BiomeIndex)
                {
                    const FOCGBiomeSettings* BiomeSettings = &MapPreset->Biomes[BiomeIndex - 1];
                    float TempDiff = FMath::Abs(BiomeSettings->Temperature - Temp) / TempRange;
                    float HumidityDiff = FMath::Abs(BiomeSettings->Humidity - Humidity);
                    float Dist = FVector2D(TempDiff, HumidityDiff).Length();
                    if (Dist < MinDist)
                    {
                        MinDist = Dist;
                        CurrentBiome = BiomeSettings;
                        CurrentBiomeIndex = BiomeIndex;
                    }
                }
            }
            
            if(CurrentBiome)
            {
                FName LayerName;
                if (CurrentBiomeIndex != INDEX_NONE)
                {
                    FString LayerNameStr = FString::Printf(TEXT("Layer%d"), CurrentBiomeIndex);
                    LayerName = FName(LayerNameStr);
                    WeightLayers[LayerName][Index] = 255;
                    BiomeNameMap[Index] = LayerName;
                    OutBiomeMap[Index] = CurrentBiome;
                    BiomeColorMap[Index] = CurrentBiome->Color.ToFColor(true);
                }
                else
                {
                    UE_LOG(LogTemp, Display, TEXT("Current Biome index is invalid"));
                }
            }
        }
    }
    ExportMap(MapPreset, BiomeColorMap, "BiomeMap.png");
    BlendBiome(MapPreset);
}

void UOCGMapGenerateComponent::BlendBiome(const UMapPreset* MapPreset)
{
    const FIntPoint CurResolution = MapPreset->MapResolution;
     // 초기화 및 원본 맵 복사
    TMap<FName, TArray<uint8>> OriginalWeightMaps;

    for (int32 LayerIndex = 0; LayerIndex < WeightLayers.Num(); ++LayerIndex)
    {
        FString LayerNameStr = FString::Printf(TEXT("Layer%d"), LayerIndex);
        FName LayerName(LayerNameStr);
        WeightLayers[LayerName].Init(0, CurResolution.X * CurResolution.Y);
        
        TArray<uint8> InitialWeights;
        InitialWeights.Init(0, CurResolution.X * CurResolution.Y);

        OriginalWeightMaps.FindOrAdd(LayerName, InitialWeights);
    }

    // 칼같이 나뉘는 초기 웨이트맵 생성 (복사본에)
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        if (OriginalWeightMaps.Contains(BiomeNameMap[i]))
        {
            OriginalWeightMaps[BiomeNameMap[i]][i] = 255;
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

    // 수평 블러 (슬라이딩 윈도우)
    int32 LayerIndex = 0;
    for (const auto& Elem : OriginalWeightMaps)
    {
        const FName& LayerName = Elem.Key;
        const TArray<uint8>& OriginalLayer = Elem.Value;
        TArray<float>& HorizontalPassLayer = HorizontalPassMaps.FindChecked(LayerName);

        int32 BlendRadius = (LayerIndex == 0) ? MapPreset->WaterBlendRadius : MapPreset->BiomeBlendRadius;
        for (int32 y = 0; y < CurResolution.Y; ++y)
        {
            float Sum = 0;
            // 첫 픽셀의 윈도우 합계 계산
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
            {
                int32 SampleX = FMath::Clamp(i, 0, CurResolution.X - 1);
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
        ++LayerIndex;
    }

    LayerIndex = 0;
    // 수직 블러 (슬라이딩 윈도우)
    for (const auto& Elem : HorizontalPassMaps)
    {
        const FName& LayerName = Elem.Key;
        const TArray<float>& HorizontalPassLayer = Elem.Value;
        TArray<uint8>& FinalLayer = *WeightLayers.Find(LayerName);

        int32 BlendRadius = (LayerIndex == 0) ? MapPreset->WaterBlendRadius : MapPreset->BiomeBlendRadius;

        const float BlendFactor = 1.f / ((BlendRadius * 2 + 1) * (BlendRadius * 2 + 1));
        
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            float Sum = 0;
            // 첫 픽셀의 윈도우 합계 계산
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
            {
                int32 SampleY = FMath::Clamp(i, 0, CurResolution.Y - 1);
                Sum += HorizontalPassLayer[SampleY * CurResolution.X  + x];
            }
            FinalLayer[x] = FMath::RoundToInt(Sum * BlendFactor);

            // 슬라이딩 윈도우
            for (int32 y = 1; y < CurResolution.Y; ++y)
            {   
                int32 OldY = FMath::Clamp(y - BlendRadius - 1, 0, CurResolution.Y - 1);
                int32 NewY = FMath::Clamp(y + BlendRadius, 0, CurResolution.Y - 1);
                Sum += HorizontalPassLayer[NewY * CurResolution.X + x] - HorizontalPassLayer[OldY * CurResolution.X + x];
                FinalLayer[y * CurResolution.X + x] = FMath::RoundToInt(Sum * BlendFactor);
            }
        }
        ++LayerIndex;
    }
    
    // 각 픽셀에 대해 모든 바이옴의 블러링된 웨이트 합이 255가 되도록 보정
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        float TotalWeight = 0;
        // TODO : 리팩토링
        for (LayerIndex = 0; LayerIndex < WeightLayers.Num(); ++LayerIndex)
        {
            FString LayerNameStr = FString::Printf(TEXT("Layer%d"), LayerIndex);
            FName LayerName(LayerNameStr);
            TotalWeight += WeightLayers[LayerName][i];
        }
        
        if (TotalWeight > 0)
        {
            float NormalizationFactor = 255.f / TotalWeight;
            for (LayerIndex = 0; LayerIndex < WeightLayers.Num(); ++LayerIndex)
            {
                FString LayerNameStr = FString::Printf(TEXT("Layer%d"), LayerIndex);
                FName LayerName(LayerNameStr);
                WeightLayers[LayerName][i] = FMath::RoundToInt(WeightLayers[LayerName][i] * NormalizationFactor);
            }
        }
    }
}

void UOCGMapGenerateComponent::ExportMap(const UMapPreset* MapPreset, const TArray<uint16>& InMap, const FString& FileName) const
{
    if (!MapPreset->bExportMaps)
        return;
    MapDataUtils::ExportMap(InMap, MapPreset->MapResolution, FileName);
}

void UOCGMapGenerateComponent::ExportMap(const UMapPreset* MapPreset, const TArray<FColor>& InMap,
    const FString& FileName) const
{
    if (!MapPreset->bExportMaps)
        return;
    MapDataUtils::ExportMap(InMap, MapPreset->MapResolution, FileName);
}
