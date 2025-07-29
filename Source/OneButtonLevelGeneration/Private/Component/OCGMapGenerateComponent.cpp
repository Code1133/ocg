// Copyright (c) 2025 Code1133. All rights reserved.

#include "Component/OCGMapGenerateComponent.h"

#include "OCGLevelGenerator.h"
#include "OCGLog.h"
#include "Data/MapData.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"


// Sets default values for this component's properties
UOCGMapGenerateComponent::UOCGMapGenerateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void UOCGMapGenerateComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UOCGMapGenerateComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

AOCGLevelGenerator* UOCGMapGenerateComponent::GetLevelGenerator() const
{
    return Cast<AOCGLevelGenerator>(GetOwner());
}

// Map Generation without imported Height Map
void UOCGMapGenerateComponent::GenerateMaps()
{
    // Display progress bar
    FScopedSlowTask SlowTask(11.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "GenerateMap", "Generating Maps"));
    SlowTask.MakeDialog(); 
    
    AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    if (!MapPreset) return;
    
    Initialize(MapPreset);

    const FIntPoint CurMapResolution = MapPreset->MapResolution;

    TArray<uint16>& HeightMapData = MapPreset->HeightMapData;
    TArray<uint16>& TemperatureMapData = MapPreset->TemperatureMapData;
    TArray<uint16>& HumidityMapData = MapPreset->HumidityMapData;
    
    // Fill Height Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Height Map"))); //Update progress bar
    GenerateHeightMap(MapPreset, CurMapResolution, HeightMapData);
    // Fill Temperature Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Temperature Map")));
    GenerateTempMap(MapPreset, HeightMapData, TemperatureMapData);
    // Fill Humidity Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Humidity Map")));
    GenerateHumidityMap(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData);
    // Decide Biome based on Height, Temperature, Humidity Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Biome Map")));
    TArray<const FOCGBiomeSettings*> BiomeMap; 
    DecideBiome(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData, BiomeMap);
    // Modify Height Map based on biome if needed
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Modifying Heightmap with Biome")));
    ModifyLandscapeWithBiome(MapPreset, HeightMapData, BiomeMap);
    // Smooth Height Map if needed
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Smoothing Height Map")));
    SmoothHeightMap(MapPreset, HeightMapData);
    // Recalculate biome based on modified height map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Finalizing Biome Map")));
    FinalizeBiome(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData, BiomeMap);
    // Erosion pass
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Working on Erosion")));
    ErosionPass(MapPreset, HeightMapData);
    // Calculate max & min height from current Height Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Calculating max and min heights")));
    GetMaxMinHeight(MapPreset, HeightMapData);
    // Export Height Map as png
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Exporting Height Map as PNG")));
    ExportMap(MapPreset, HeightMapData, "HeightMap.png");
    // End progress bar
    SlowTask.EnterProgressFrame();
}

// Map Generation with imported Height Map
void UOCGMapGenerateComponent::GenerateMapsWithHeightMap()
{
    // Display Progress bar
    FScopedSlowTask SlowTask(6.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "GenerateMap", "Generating Maps"));
    SlowTask.MakeDialog();
    
    AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    if (!LevelGenerator || !LevelGenerator->GetMapPreset())
        return;
    
    UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    if (!MapPreset) return;

    Initialize(MapPreset);

    TArray<uint16>& HeightMapData = MapPreset->HeightMapData;
    TArray<uint16>& TemperatureMapData = MapPreset->TemperatureMapData;
    TArray<uint16>& HumidityMapData = MapPreset->HumidityMapData;
    
    // Generate Temperature Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Temperature Map")));
    GenerateTempMap(MapPreset, HeightMapData, TemperatureMapData);
    // Generate Humidity Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Humidity Map")));
    GenerateHumidityMap(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData);
    // Decide Biome based on Height, Temperature, Humidity Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Generating Biome Map")));
    TArray<const FOCGBiomeSettings*> BiomeMap; 
    DecideBiome(MapPreset, HeightMapData, TemperatureMapData, HumidityMapData, BiomeMap, true);
    // Calculate max & min height from current Height Map
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Calculating max and min heights")));
    GetMaxMinHeight(MapPreset, HeightMapData);
    // Export Height Map as png
    SlowTask.EnterProgressFrame(1.0f, FText::FromString(TEXT("Exporting Height Map as PNG")));
    ExportMap(MapPreset, HeightMapData, "HeightMap.png");
    // End progress bar
    SlowTask.EnterProgressFrame();
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
    // Add ZOffset to return actual world Height, ZOffset is 0 if absolute value of max & min height is same
    return (Height - 32768.f) * LandscapeZScale / 128.f + ZOffset;
}

uint16 UOCGMapGenerateComponent::WorldHeightToHeightMap(float Height)
{
    // Subtract ZOffset to return actual Height Map value, ZOffset is 0 if absolute value of max & min height is same
    return static_cast<uint16>((Height - ZOffset) * 128.f / LandscapeZScale + 32768.f);
}

void UOCGMapGenerateComponent::Initialize(const UMapPreset* MapPreset)
{
    Stream.Initialize(MapPreset->Seed);
    
    NoiseScale = 1;
    if (MapPreset->ApplyScaleToNoise)
    {
        // Alter noise scale based on LandscapeScale, use log so that scale does not increase linearly
        // Linearly increasing scale results in too high scale
        float LandscapeScale = MapPreset->LandscapeScale; 
        if (LandscapeScale > 0.f)
            NoiseScale = FMath::LogX(25.f, LandscapeScale) + 1;
    }
    
    InitializeNoiseOffsets(MapPreset);

    // Set height of Plain to just above sea level
    if (MapPreset->bContainWater)
    {
        PlainHeight = MapPreset->SeaLevel * 1.005f;
    }
    else
    {
        // bContainWater is false plain height is min height
        PlainHeight = 0.f;
    }

    // Landscape scale formula, works only when absolute value of max & min height is same
    // To solve this the landscape is moved in Z direction by ZOffset
    LandscapeZScale = (MapPreset->MaxHeight - MapPreset->MinHeight) * 0.001953125f;

    float AbsMaxHeight = FMath::Abs(MapPreset->MaxHeight);
    float AbsMinHeight = FMath::Abs(MapPreset->MinHeight);
    float AbsOffset = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;

    ZOffset = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;
    
    WeightLayers.Empty();
}

void UOCGMapGenerateComponent::InitializeNoiseOffsets(const UMapPreset* MapPreset)
{
    float StandardNoiseOffset = MapPreset->StandardNoiseOffset * NoiseScale;
    PlainNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    PlainNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);

    MountainNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    MountainNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    
    BlendNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    BlendNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);

    DetailNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    DetailNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);

    IslandNoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
    IslandNoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
}

void UOCGMapGenerateComponent::GenerateHeightMap(const UMapPreset* MapPreset, const FIntPoint CurMapResolution, TArray<uint16>& OutHeightMap)
{
    OutHeightMap.SetNumUninitialized(CurMapResolution.X * CurMapResolution.Y);
    
    // Fill Height Map
    for (int32 y = 0; y < CurMapResolution.Y; ++y)
    {
        for (int32 x = 0; x <CurMapResolution.X; ++x)
        {
            // returns value between 0~1
            const float CalculatedHeight = CalculateHeightForCoordinate(MapPreset, x, y);
            // convert 0~1 value to 0~65535 which is the range of height map
            const float NormalizedHeight = CalculatedHeight * 65535.f;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            OutHeightMap[y * CurMapResolution.X + x] = HeightValue;
        }
    }
}

float UOCGMapGenerateComponent:: CalculateHeightForCoordinate(const UMapPreset* MapPreset, const int32 InX, const int32 InY) const
{
	// 1. Use Low frequency noise to generate large mountains
    float MountainNoiseX = InX * MapPreset->ContinentNoiseScale * NoiseScale + MountainNoiseOffset.X;
    float MountainNoiseY = InY * MapPreset->ContinentNoiseScale * NoiseScale + MountainNoiseOffset.Y;
    // -1~1 Range
    float MountainHeight = FMath::PerlinNoise2D(FVector2D(MountainNoiseX, MountainNoiseY));
    MountainHeight *= 2.f;
    MountainHeight = FMath::Clamp(MountainHeight, -1.f, 1.f);

    // 2. Add details using high frequency to add details to Mountains generated in step 1
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
    // Normalize Terrain Noise (-1~1)
    TerrainNoise /= MaxPossibleAmplitude;
    TerrainNoise *= 0.3f;
    MountainHeight += TerrainNoise;
    MountainHeight = FMath::Clamp(MountainHeight, -1.f, 1.f);
    
    // 3. Generate Blend mask that blends mountain and plain
    float BlendNoiseX = InX * MapPreset->ContinentNoiseScale * NoiseScale + BlendNoiseOffset.X;
    float BlendNoiseY = InY * MapPreset->ContinentNoiseScale * NoiseScale + BlendNoiseOffset.Y;
    // 0~1 Range
    float BlendNoise = FMath::PerlinNoise2D(FVector2D(BlendNoiseX, BlendNoiseY)) * 0.5f + 0.5f;
    // Redistribute BlendNoise so that Mountain region and Plain region is clear
    if (MapPreset->RedistributionFactor > 1.f && BlendNoise > 0.f && BlendNoise < 1.f)
    {
        float PowX = FMath::Pow(BlendNoise, MapPreset->RedistributionFactor);
        float Pow1_X = FMath::Pow(1-BlendNoise, MapPreset->RedistributionFactor);
        BlendNoise = PowX/(PowX + Pow1_X);
    }
    BlendNoise = FMath::SmoothStep(0.f, 1.f, BlendNoise);

    // 4. Apply Blend mask and blend heights
    MountainHeight = MountainHeight * 0.5f + 0.5f; // Change range of Mountain height from -1~1 to 0~1
    float Height = FMath::Lerp(PlainHeight, MountainHeight, BlendNoise);
    Height = FMath::Clamp(Height, 0.f, 1.f);

    // 5. Apply Island shape
    if (MapPreset->bIsland)
    {
        // Calculate current pixels distance from center of landscape;
        float nx = (static_cast<float>(InX) / MapPreset->MapResolution.X) * 2.f - 1.f;
        float ny = (static_cast<float>(InY) / MapPreset->MapResolution.Y) * 2.f - 1.f;
        float Distance = FMath::Sqrt(nx * nx + ny * ny);
        // Generate coastline noise so that island is not perfect circle
        float IslandNoiseX = InX * MapPreset->IslandShapeNoiseScale * NoiseScale + IslandNoiseOffset.X;
        float IslandNoiseY = InY * MapPreset->IslandShapeNoiseScale * NoiseScale + IslandNoiseOffset.Y;
        // -1~1 range
        float IslandNoise = FMath::PerlinNoise2D(FVector2D(IslandNoiseX, IslandNoiseY));
        // Apply IslandNoise to Distance so that coastline is random
        float DistortedDistance = Distance + IslandNoise * MapPreset->IslandShapeNoiseStrength;
        // Generate final Island mask and apply to height map
        float IslandMask = 1.f - DistortedDistance;
        // multiply IslandMask by 3 so that the distribution shifts closer to 1 and more land will be shown
        IslandMask *= 3.f;
        IslandMask = FMath::Clamp(IslandMask, 0.f, 1.f);
        IslandMask = FMath::Pow(IslandMask, MapPreset->IslandFalloffExponent);
        IslandMask = FMath::SmoothStep(0.f, 1.0f, IslandMask);
        IslandMask = FMath::Clamp(IslandMask, 0.f, 1.f);
        Height *= IslandMask;
        Height = FMath::Clamp(Height, 0.f, 1.f);
    }

    return Height;
}

void UOCGMapGenerateComponent::ErosionPass(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap)
{
    if (MapPreset->NumErosionIterations <= 0 || !MapPreset->bErosion) return;

    // 1. Initialize erosion brush which generates brush according to erosion radius
    InitializeErosionBrush();
    
    // 2. Change Height Map from uint16 to world height float
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

    // 3. Main Erosion loop
    for (int i = 0; i < MapPreset->NumErosionIterations; ++i)
    {
        // initialize droplets
        float PosX = Stream.RandRange(1.f, MapPreset->MapResolution.X - 2.f);
        float PosY = Stream.RandRange(1.f, MapPreset->MapResolution.Y - 2.f);
        float DirX = 0, DirY = 0;
        float Speed = MapPreset->InitialSpeed;
        float Water = MapPreset->InitialWaterVolume;
        float Sediment = 0;

        // simulate droplet
        for (int Lifetime = 0; Lifetime <MapPreset-> MaxDropletLifetime; ++Lifetime)
        {
            int32 NodeX = static_cast<int32>(PosX);
            int32 NodeY = static_cast<int32>(PosY);
            int32 DropletIndex = NodeY * MapPreset->MapResolution.X + NodeX;

            // if droplet is out of map end simulation
            if (NodeX < 0 || NodeX >= MapPreset->MapResolution.X - 1 || NodeY < 0 || NodeY >= MapPreset->MapResolution.Y - 1)
            {
                break;
            }

            // Calculate current pixels height and gradient
            FVector2D Gradient;
            float CurrentHeight = CalculateHeightAndGradient(MapPreset, HeightMapFloat, LandscapeScale, PosX, PosY, Gradient);
            
            // Apply inertia and calculate droplets direction
            DirX = (DirX * MapPreset->DropletInertia) - (Gradient.X * (1 - MapPreset->DropletInertia));
            DirY = (DirY * MapPreset->DropletInertia) - (Gradient.Y * (1 - MapPreset->DropletInertia));
            
            // Normalize direction
            float Len = FMath::Sqrt(DirX * DirX + DirY * DirY);
            if (Len > KINDA_SMALL_NUMBER)
            {
                DirX /= Len;
                DirY /= Len;
            }
            
            // Move to new pixel using calculated direction
            PosX += DirX;
            PosY += DirY;
            
            if (PosX <= 0 || PosX >= MapPreset->MapResolution.X - 1 ||
                PosY <= 0 || PosY >= MapPreset->MapResolution.Y - 1)
            {
                break;
            }
            
            // calculate height and gradient from new pixel
            float NewHeight = CalculateHeightAndGradient(MapPreset, HeightMapFloat, LandscapeScale, PosX, PosY, Gradient);
            if (NewHeight <= SeaLevelHeight)
                break;
            float HeightDifference = NewHeight - CurrentHeight;
            
            // calculate sediment capacity
            float SedimentCapacity = FMath::Max(-HeightDifference * Speed * Water * MapPreset->SedimentCapacityFactor, MapPreset->MinSedimentCapacity);
            
            // apply sediment or erosion
            if (Sediment > SedimentCapacity || HeightDifference > 0)
            {
                // sediment
                float AmountToDeposit = (HeightDifference > 0) ? FMath::Min(Sediment, HeightDifference) : (Sediment - SedimentCapacity) * MapPreset->DepositSpeed;
                Sediment -= AmountToDeposit;
                
                // apply sediment using pre-calculated erosion brush
                const TArray<int32>& Indices = ErosionBrushIndices[DropletIndex];
                const TArray<float>& Weights = ErosionBrushWeights[DropletIndex];
                for (int j = 0; j < Indices.Num(); j++)
                {
                    HeightMapFloat[Indices[j]] += AmountToDeposit * Weights[j];
                }
            }
            else
            {
                // erosion
                float AmountToErode = FMath::Min((SedimentCapacity - Sediment), -HeightDifference) * MapPreset->ErodeSpeed;
                
                // apply erosion using pre-calculated erosion brush
                const TArray<int32>& Indices = ErosionBrushIndices[DropletIndex];
                const TArray<float>& Weights = ErosionBrushWeights[DropletIndex];
                for (int j = 0; j < Indices.Num(); j++)
                {
                    HeightMapFloat[Indices[j]] -= AmountToErode * Weights[j];
                }
                Sediment += AmountToErode;
            }
            
            // update droplets water amount and speed
            Speed = FMath::Sqrt(FMath::Max(0.f, Speed * Speed - HeightDifference * MapPreset->Gravity));
            Water *= (1.0f - MapPreset->EvaporateSpeed);
        }
    }
    uint16 SeaHeight = WorldHeightToHeightMap(SeaLevelHeight);
    // 4. change world height map to height map (uint16)
    for (int i = 0; i < HeightMapFloat.Num(); ++i)
    {
        // erase height higher than original height map due to sediment
        uint16 Height =  WorldHeightToHeightMap(HeightMapFloat[i]);
        uint16 NewHeight = FMath::Min(Height, InOutHeightMap[i]);
        // prevent height from going under sea level due to erosion
        if (InOutHeightMap[i] >= SeaHeight)
            NewHeight = FMath::Max(NewHeight, SeaHeight);
        InOutHeightMap[i] = FMath::Clamp(NewHeight, 0, 65535);
    }
}

void UOCGMapGenerateComponent::InitializeErosionBrush()
{
    const AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    // if current erosion brush is initialized with current map resolution and erosion radius
    int NewSize = MapPreset->MapResolution.X * MapPreset->MapResolution.Y;
    if (CurrentErosionRadius == MapPreset->ErosionRadius && ErosionBrushIndices.Num() == NewSize)
    {
        return;
    }

    ErosionBrushIndices.Empty();
    ErosionBrushWeights.Empty();
    
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
                        int Index = CoordY * MapPreset->MapResolution.X + CoordX;
                        Index = FMath::Clamp(Index, 0, MapPreset->MapResolution.X * MapPreset->MapResolution.Y - 1);
                        float Weight = 1.0f - (Dist / MapPreset->ErosionRadius);
                        WeightSum += Weight;
                        Indices.Add(Index);
                        Weights.Add(Weight);
                    }
                }
            }
        }
        
        // Normalize Weights so that weight sum at any index is 1
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
    
    float x = PosX - CoordX;
    float y = PosY - CoordY;

    // 4 close indices
    int32 Index_00 = CoordY * MapPreset->MapResolution.X + CoordX; // Closest index
    int32 Index_10 = Index_00 + 1; // index of pixel at right
    int32 Index_01 = Index_00 + MapPreset->MapResolution.X; // index of pixel at bottom
    int32 Index_11 = Index_01 + 1; // index of pixel at bottom right

    // Heights at each index
    float Height_00 = HeightMap[Index_00];
    float Height_10 = HeightMap[Index_10];
    float Height_01 = HeightMap[Index_01];
    float Height_11 = HeightMap[Index_11];

    // Calculate Gradient
    OutGradient.X = ((Height_10 - Height_00) * (1 - y) + (Height_11 - Height_01)) * y / LandscapeScale;
    OutGradient.Y = ((Height_01 - Height_00) * (1 - x) + (Height_11 - Height_10)) * x / LandscapeScale;

    // Calculate and return Height
    return Height_00 * (1 - x) * (1 - y) + Height_10 * x * (1 - y) + Height_01 * (1 - x) * y + Height_11 * x * y;
}

void UOCGMapGenerateComponent::ModifyLandscapeWithBiome(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap)
{
    if (!MapPreset->bModifyTerrainByBiome)
        return;
    TArray<float> MinHeights;
    TArray<float> BlurredMinHeights;

    float HeightRange = MapPreset->MaxHeight - MapPreset->MinHeight;
    
    // Calculate each Biome region's minimum height
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
    float SeaLevelHeightF = SeaLevel * HeightRange + MapPreset->MinHeight;
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
            // Apply blur
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
            // Plain Height of the biome region
            uint16 TargetPlainHeight = FMath::Lerp(CurrentHeight, BiomeMinHeight, (1.0f - MtoPRatio) * MapPreset->PlainSmoothFactor);
            // Generate Mountain Noise
            float MaxAmplitude = (65535 - TargetPlainHeight) * LandscapeZScale / HeightRange / 128.f;
            float Amplitude = MaxAmplitude * MapPreset->BiomeNoiseAmplitude;
            float DetailNoise = FMath::PerlinNoise2D(FVector2D(static_cast<float>(x), static_cast<float>(y)) *
                MapPreset->BiomeNoiseScale) * Amplitude + Amplitude;
            float HeightToAdd = DetailNoise * HeightRange * 128.f / LandscapeZScale;
            float MountainHeight = FMath::Clamp(HeightToAdd + TargetPlainHeight, 0, 65535);

            // Calculate final biome height by lerp
            uint16 NewHeight = FMath::Lerp(TargetPlainHeight, MountainHeight, MtoPRatio);
            NewHeight = FMath::Clamp(FMath::Max(NewHeight, SeaLevelHeight), 0, 65535);
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
    // Horizontal Pass
    TArray<float> HorizontalPass;
    HorizontalPass.Init(0, TotalPixels);
    for (int32 y = 0; y < MapSize.Y; ++y)
    {
        float Sum = 0;
        int32 ValidPixelCount = 0;
        // calculate first pixel
        for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
        {
            const int32 CurrentX = FMath::Clamp(i, 0, MapSize.X - 1);
            const int32 Index = y * MapSize.X + CurrentX;
            Sum+=InMinHeights[Index];
            ValidPixelCount++;
        }
        HorizontalPass[y*MapSize.X + 0] = (ValidPixelCount > 0) ? Sum / ValidPixelCount : InMinHeights[y*MapSize.X + 0];
        // move on to next pixels
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
        // initial pixel
        for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
        {
            const int32 CurrentY = FMath::Clamp(i, 0, MapSize.Y - 1);
            const int32 Index = CurrentY * MapSize.X + x;
            Sum+=HorizontalPass[Index];
            ValidPixelCount++;
        }
        OutMinHeights[0 * MapSize.X + x] = (ValidPixelCount > 0) ? Sum / ValidPixelCount : HorizontalPass[0 * MapSize.X + x];

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

void UOCGMapGenerateComponent::GetMaxMinHeight(UMapPreset* MapPreset, const TArray<uint16>& InHeightMap)
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
    MapPreset->CurMaxHeight = Max;
    MapPreset->CurMinHeight = Min;
}

void UOCGMapGenerateComponent::SmoothHeightMap(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap)
{
    if (!MapPreset->bSmoothHeight)
        return;
    
    ApplySpikeSmooth(MapPreset, InOutHeightMap);
    
    TArray<uint16> BlurredHeightMap;
    
    ApplyGaussianBlur(MapPreset, InOutHeightMap, BlurredHeightMap);

    MedianSmooth(MapPreset, InOutHeightMap);
}

void UOCGMapGenerateComponent::ApplyGaussianBlur(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap,
    TArray<uint16>& OutBlurredMap)
{
    int32 Radius = MapPreset->GaussianBlurRadius;

    FIntPoint MapSize = MapPreset->MapResolution;
    int32 TotalPixels = MapSize.X * MapSize.Y;

    OutBlurredMap.SetNumUninitialized(TotalPixels);
    
    TArray<float> TempMap;
    TempMap.SetNum(TotalPixels);

    // Horizontal Blur
    for (int32 y=0; y<MapSize.Y; y++)
    {
        for (int32 x=0; x<MapSize.X; x++)
        {
            float Sum = 0;
            float WeightSum = 0;
            for (int32 i=-Radius; i<=Radius; i++)
            {
                int32 SampleX = FMath::Clamp(x+i, 0, MapSize.X-1);
                int32 Index = y*MapSize.X + SampleX;
                float Weight = FMath::Exp(-(i*i) / (2.f * Radius * Radius));
                Sum += InOutHeightMap[Index] * Weight;
                WeightSum += Weight;
            }
            TempMap[y*MapSize.X + x] = Sum / WeightSum;
        }
    }

    // Vertical Blur
    for (int32 x = 0; x < MapSize.X; ++x)
    {
        for (int32 y = 0; y < MapSize.Y; ++y)
        {
            float Sum = 0;
            float WeightSum = 0;
            for (int32 i = -Radius; i <= Radius; ++i)
            {
                int32 SampleY = FMath::Clamp(y + i, 0, MapSize.Y - 1);
                int32 Index = SampleY * MapSize.X + x;
                float Weight = FMath::Exp(-(i * i) / (2.0f * Radius * Radius));
                Sum += TempMap[Index] * Weight;
                WeightSum += Weight;
            }
            OutBlurredMap[y * MapSize.X + x] = FMath::Clamp(FMath::RoundToInt(Sum / WeightSum), 0, 65535);
        }
    }

    InOutHeightMap = OutBlurredMap;
}

void UOCGMapGenerateComponent::ApplySpikeSmooth(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap)
{
    if (!MapPreset->bSmoothBySlope)
        return;
    
    FIntPoint MapSize = MapPreset->MapResolution;

    const int32 KernelRadius = MapPreset->GaussianBlurRadius / 2.f;
    const int32 KernelSize = (2 * KernelRadius + 1);
    
    const float MaxAllowedSlope = FMath::Tan(FMath::DegreesToRadians(MapPreset->MaxSlopeAngle));

    int32 Step = FMath::Max(KernelRadius / 2.f, 1);
    
    for (int Iteration=0; Iteration<MapPreset->SmoothingIteration; Iteration++)
    {
        int32 SmoothedRegion = 0;
        TArray<uint16> OriginalHeightMap = InOutHeightMap; 
        for (int32 y=KernelRadius; y<MapSize.Y-KernelRadius; y += Step)
        {
            for (int32 x=KernelRadius; x<MapSize.X-KernelRadius; x += Step)
            {
                ProcessPlane(MapPreset, x, y, MapSize, KernelRadius, KernelSize, MaxAllowedSlope,
                SmoothedRegion, OriginalHeightMap, InOutHeightMap);
            }
        }
        if (SmoothedRegion == 0)
            break;
    }
}

void UOCGMapGenerateComponent::ProcessPlane(const UMapPreset* MapPreset, int32 x, int32 y, const FIntPoint MapSize,
    const int32 KernelRadius, const int32 KernelSize, const float MaxAllowedSlope,
    int32& SmoothedRegion, TArray<uint16>& InOriginalHeightMap, TArray<uint16>& OutHeightMap)
{
    float LandscapeScale = MapPreset->LandscapeScale * 100.f;

    const float Length = KernelSize * LandscapeScale;
    
    float TLHeight = HeightMapToWorldHeight(InOriginalHeightMap[(y-KernelRadius)*MapSize.X + (x-KernelRadius)]);
    float TRHeight = HeightMapToWorldHeight(InOriginalHeightMap[(y-KernelRadius)*MapSize.X + (x+KernelRadius)]);
    float BLHeight = HeightMapToWorldHeight(InOriginalHeightMap[(y+KernelRadius)*MapSize.X + (x-KernelRadius)]);
    float BRHeight = HeightMapToWorldHeight(InOriginalHeightMap[(y+KernelRadius)*MapSize.X + (x+KernelRadius)]);

    float TopSlope = (TRHeight - TLHeight)/Length;
    float BottomSlope = (BRHeight - BLHeight)/Length;
    float RightSlope = (BRHeight - TRHeight)/Length;
    float LeftSlope = (BLHeight - TLHeight)/Length;

    float Slope_X = (BottomSlope + TopSlope)/2.f;
    float Slope_Y = (RightSlope + LeftSlope)/2.f;

    const float CurrentSlope = FMath::Sqrt(Slope_X * Slope_X + Slope_Y * Slope_Y);
    
    if (CurrentSlope > MaxAllowedSlope)
    {
        SmoothedRegion++;

        float AverageHeight = (TLHeight + TRHeight + BRHeight + BLHeight)/4.f;
        FVector Plane = {Slope_X, Slope_Y, AverageHeight};

        float CorrectionFactor = MaxAllowedSlope / CurrentSlope;
        float CorrectedSlope_X = Slope_X * CorrectionFactor;
        float CorrectedSlope_Y = Slope_Y * CorrectionFactor;

        for (int32 ky = -KernelRadius; ky<=KernelRadius; ky++)
        {
            for (int32 kx = -KernelRadius; kx<=KernelRadius; kx++)
            {
                int32 Index = (y+ky)*MapSize.X + (x+kx);
                float OriginalWorldHeight = HeightMapToWorldHeight(InOriginalHeightMap[Index]);
                float CurrentHeight = Plane.X * kx * LandscapeScale + Plane.Y * ky * LandscapeScale + Plane.Z;
                float CorrectedHeight = CorrectedSlope_X * kx * LandscapeScale + CorrectedSlope_Y * ky * LandscapeScale + Plane.Z;
                float NewWorldHeight = OriginalWorldHeight + (CorrectedHeight - CurrentHeight);
                NewWorldHeight = FMath::Clamp(NewWorldHeight, MapPreset->MinHeight + ZOffset, MapPreset->MaxHeight + ZOffset);
                uint16 NewHeight = WorldHeightToHeightMap(NewWorldHeight);
                uint16 OriginalHeight = WorldHeightToHeightMap(OriginalWorldHeight);

                OutHeightMap[Index] = FMath::Lerp(OriginalHeight, NewHeight, MapPreset->SmoothingStrength);
            }
        }
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
            
            // Generate base temperature map with low frequency noise
            float TempNoiseInputX = x * MapPreset->TemperatureNoiseScale + PlainNoiseOffset.X;
            float TempNoiseInputY = y * MapPreset->TemperatureNoiseScale + PlainNoiseOffset.Y;
            
            float TempNoiseAlpha = FMath::PerlinNoise2D(FVector2D(TempNoiseInputX, TempNoiseInputY)) * 0.5f + 0.5f;
            
            float BaseTemp = FMath::Lerp(MapPreset->MinTemp, MapPreset->MaxTemp, TempNoiseAlpha);

            // Decrease temperature by altitude
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

            BaseTemp = MapPreset->MinTemp + NormalizedBaseTemp * TempRange;
            // Calculate final temperature
            const float FinalTemp = FMath::Clamp(BaseTemp, MapPreset->MinTemp, MapPreset->MaxTemp);
            
            TempMapFloat[Index] = FinalTemp;
            
            if (FinalTemp < GlobalMinTemp) GlobalMinTemp = FinalTemp;
            if (FinalTemp > GlobalMaxTemp) GlobalMaxTemp = FinalTemp;
        }
    }

    CachedGlobalMinTemp = GlobalMinTemp;
    CachedGlobalMaxTemp = GlobalMaxTemp;

    // convert float temperature to uint16
    TempRange = GlobalMaxTemp - GlobalMinTemp;
    if (TempRange < KINDA_SMALL_NUMBER)
    {
        TempRange = 1.0f; // prevent dividing by 0
    }

    for (int32 i = 0; i < TempMapFloat.Num(); ++i)
    {
        // Normalize temperature to 0~1
        const float NormalizedTemp = (TempMapFloat[i] - GlobalMinTemp) / TempRange;
        
        // convert 0~1 to 0~65535
        OutTempMap[i] = static_cast<uint16>(NormalizedTemp * 65535.0f);
    }

    // Export as png
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
    
    TArray<float> HumidityMapFloat;
    HumidityMapFloat.Init(0.0f, CurResolution.X * CurResolution.Y);
    
    TArray<float> DistanceToWater;
    DistanceToWater.Init(TNumericLimits<float>::Max(), CurResolution.X * CurResolution.Y);
    
    // 1. Find water pixels and Enqueue them
    TQueue<FIntPoint> Frontier; 
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X  + x;
            const float WorldHeight = HeightMapToWorldHeight(InHeightMap[Index]);

            if (WorldHeight <= SeaLevelWorldHeight)
            {
                DistanceToWater[Index] = 0; // water pixels' distance from water is 0
                Frontier.Enqueue(FIntPoint(x, y));
            }
        }
    }

    // Use BFS to calculate closest distance to water
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
                // if Neighbor is unvisited (distance to water is initial value)
                if (DistanceToWater[NeighborIndex] == TNumericLimits<float>::Max())
                {
                    DistanceToWater[NeighborIndex] = DistanceToWater[Current.Y * CurResolution.X + Current.X] + 1.0f;
                    Frontier.Enqueue(FIntPoint(nx, ny));
                }
            }
        }
    }

    // 2. Calculate humidity based on distance and temperature
    float GlobalMinHumidity = TNumericLimits<float>::Max();
    float GlobalMaxHumidity = TNumericLimits<float>::Lowest();
    
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        float FinalHumidity = 0.0f;
        
        if (DistanceToWater[i] == 0)
        {
            // water pixel's humidity is always 1
            FinalHumidity = 1.0f; 
        }
        else
        {
            // decide humidity based on distance
            const float HumidityFromDistance = FMath::Exp(-DistanceToWater[i] * MapPreset->MoistureFalloffRate);
        
            // apply temperature affect
            const float NormalizedTemp = static_cast<float>(InTempMap[i]) / 65535.0f;
            FinalHumidity = HumidityFromDistance * (1.0f - (NormalizedTemp * MapPreset->TemperatureInfluenceOnHumidity));
        }

        FinalHumidity = FMath::Clamp(FinalHumidity, 0.0f, 1.0f);
        
        HumidityMapFloat[i] = FinalHumidity;
        
        if (FinalHumidity < GlobalMinHumidity) GlobalMinHumidity = FinalHumidity;
        if (FinalHumidity > GlobalMaxHumidity) GlobalMaxHumidity = FinalHumidity;
    }

    CachedGlobalMinHumidity = GlobalMinHumidity;
    CachedGlobalMaxHumidity = GlobalMaxHumidity;
    
    // 3. convert to uint16 data
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
    const TArray<uint16>& InHumidityMap, TArray<const FOCGBiomeSettings*>& OutBiomeMap, bool bExportMap)
{
    float TotalWeight = 0.f;
    for (const auto Biome : MapPreset->Biomes)
    {
        TotalWeight += Biome.Weight;
    }
    
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
    // Water biome
    TArray<uint8> WaterWeightLayer;
    WaterWeightLayer.SetNumZeroed(CurResolution.X * CurResolution.Y);
    FString WaterLayerNameStr = FString::Printf(TEXT("Layer%d"), 0);
    FName WaterLayerName(WaterLayerNameStr);
    WeightLayers.Add(WaterLayerName, WaterWeightLayer);

    uint16 SeaLevelHeight;
    if (MapPreset->bContainWater)
    {
        SeaLevelHeight = 65535 * MapPreset->SeaLevel;
    }
    else
    {
        SeaLevelHeight = 0;
    }
    
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X + x;
            float Height = InHeightMap[Index];
            float NormalizedTemp = static_cast<float>(InTempMap[y * CurResolution.X + x]) / 65535.f;
            const float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y * CurResolution.X + x]) / 65535.f;
            const float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            const FOCGBiomeSettings* CurrentBiome = nullptr;
            const FOCGBiomeSettings* WaterBiome = &MapPreset->WaterBiome;
            uint32 CurrentBiomeIndex = INDEX_NONE;
            if (WaterBiome && Height < SeaLevelHeight)
            {
                CurrentBiome = WaterBiome;
                // Water biome is first layer
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
                    float Weight = 1.f - BiomeSettings->Weight / TotalWeight;
                    float Dist = FVector2D(TempDiff, HumidityDiff).Length() * Weight;
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
                    UE_LOG(LogOCGModule, Display, TEXT("Current Biome index is invalid"));
                }
            }
        }
    }
    if (bExportMap)
        ExportMap(MapPreset, BiomeColorMap, "BiomeMap1.png");
    BlendBiome(MapPreset);
}

void UOCGMapGenerateComponent::FinalizeBiome(const UMapPreset* MapPreset, const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
    const TArray<uint16>& InHumidityMap, TArray<const FOCGBiomeSettings*>& OutBiomeMap)
{
    if (!MapPreset->bContainWater)
        return;

    float TotalWeight = 0.0f;
    for (const auto Biome : MapPreset->Biomes)
    {
        TotalWeight += Biome.Weight;
    }
    
    const FIntPoint CurResolution = MapPreset->MapResolution;

    uint16 SeaLevelHeight = 65535 * MapPreset->SeaLevel;
    
    for (int32 y = 0; y < CurResolution.Y; ++y)
    {
        for (int32 x = 0; x < CurResolution.X; ++x)
        {
            const int32 Index = y * CurResolution.X + x;
            float Height = InHeightMap[Index];
            float NormalizedTemp = static_cast<float>(InTempMap[y * CurResolution.X + x]) / 65535.f;
            const float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y * CurResolution.X + x]) / 65535.f;
            const float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            const FOCGBiomeSettings* CurrentBiome = nullptr;
            const FOCGBiomeSettings* WaterBiome = &MapPreset->WaterBiome;
            uint32 CurrentBiomeIndex = INDEX_NONE;
            if (WaterBiome && Height < SeaLevelHeight)
            {
                CurrentBiome = WaterBiome;
                // Water Biome is first layer
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
                    float Weight = 1.f - BiomeSettings->Weight / TotalWeight;
                    float Dist = FVector2D(TempDiff, HumidityDiff).Length() * Weight;
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
                    UE_LOG(LogOCGModule, Display, TEXT("Current Biome index is invalid"));
                }
            }
        }
    }
    ExportMap(MapPreset, BiomeColorMap, "BiomeMap1.png");
    BlendBiome(MapPreset);

    for (int LayerIndex = 0; LayerIndex < WeightLayers.Num(); ++LayerIndex)
    {
        FString LayerNameStr = FString::Printf(TEXT("Layer%d"), LayerIndex);
        FName LayerName(LayerNameStr);
        FString FileName = LayerNameStr + ".png";
        
        if (MapPreset->bExportMapTextures)
        {
            OCGMapDataUtils::ExportMap(WeightLayers.FindRef(LayerName), CurResolution, FileName);
        }
    }
}

void UOCGMapGenerateComponent::MedianSmooth(const UMapPreset* MapPreset, TArray<uint16>& InOutHeightMap)
{
    if (!MapPreset->bSmoothByMediumHeight)
        return;
    const int32 Radius = MapPreset->MedianSmoothRadius;
    const FIntPoint MapSize = MapPreset->MapResolution;

    TArray<uint16> OriginalHeightMap = InOutHeightMap;
    TArray<uint16> Window;
    Window.Reserve((Radius * 2 + 1) * (Radius * 2 + 1));

    for (int32 y = Radius; y < MapSize.Y - Radius; ++y)
    {
        for (int32 x = Radius; x < MapSize.X - Radius; ++x)
        {
            Window.Reset();
            // Collect neighbor pixel's height
            for (int32 wy = -Radius; wy <= Radius; ++wy)
            {
                for (int32 wx = -Radius; wx <= Radius; ++wx)
                {
                    Window.Add(OriginalHeightMap[(y + wy) * MapSize.X + (x + wx)]);
                }
            }

            // Sort height values
            Window.Sort();

            // Use median height as current pixel's height
            InOutHeightMap[y * MapSize.X + x] = Window[Window.Num() / 2];
        }
    }
}

void UOCGMapGenerateComponent::BlendBiome(const UMapPreset* MapPreset)
{
    const FIntPoint CurResolution = MapPreset->MapResolution;
     // initialize and copy original map
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

    // generate unblurred map
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        if (OriginalWeightMaps.Contains(BiomeNameMap[i]))
        {
            OriginalWeightMaps[BiomeNameMap[i]][i] = 255;
        }
    }
    
    TMap<FName, TArray<float>> HorizontalPassMaps;
    for (const auto& Elem : OriginalWeightMaps)
    {
        HorizontalPassMaps.Add(Elem.Key, TArray<float>());
        HorizontalPassMaps.FindChecked(Elem.Key).Init(0.f, CurResolution.X * CurResolution.Y);
    }

    // Horizontal blur
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
            // first pixel
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
            {
                int32 SampleX = FMath::Clamp(i, 0, CurResolution.X - 1);
                Sum += OriginalLayer[y * CurResolution.X  + SampleX];
            }
            HorizontalPassLayer[y * CurResolution.X  + 0] = Sum;
            
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
    // Vertical blur
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
            for (int32 i = -BlendRadius; i <= BlendRadius; ++i)
            {
                int32 SampleY = FMath::Clamp(i, 0, CurResolution.Y - 1);
                Sum += HorizontalPassLayer[SampleY * CurResolution.X  + x];
            }
            FinalLayer[x] = FMath::RoundToInt(Sum * BlendFactor);
            
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
    
    // make sure each pixel's weight sum is equal to 255
    for (int32 i = 0; i < CurResolution.X * CurResolution.Y; ++i)
    {
        float TotalWeight = 0;
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
    if (MapPreset->bExportMapTextures)
    {
        OCGMapDataUtils::ExportMap(InMap, MapPreset->MapResolution, FileName);
    }
}

void UOCGMapGenerateComponent::ExportMap(const UMapPreset* MapPreset, const TArray<FColor>& InMap,
    const FString& FileName) const
{
    if (MapPreset->bExportMapTextures)
    {
        OCGMapDataUtils::ExportMap(InMap, MapPreset->MapResolution, FileName);
    }
}
