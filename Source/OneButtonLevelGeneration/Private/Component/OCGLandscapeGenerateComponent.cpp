// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/OCGLandscapeGenerateComponent.h"
#include "EngineUtils.h"
#include "Data/OCGBiomeSettings.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeSettings.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#endif

class IImageWrapper;
// Sets default values for this component's properties
UOCGLandscapeGenerateComponent::UOCGLandscapeGenerateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UOCGLandscapeGenerateComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UOCGLandscapeGenerateComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                   FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UOCGLandscapeGenerateComponent::Generate()
{
	const FRandomStream Stream(Seed);
	NoiseOffset.X = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	NoiseOffset.Y = Stream.FRandRange(-StandardNoiseOffset, StandardNoiseOffset);
	GenerateLandscape();
}

float UOCGLandscapeGenerateComponent::CalculateHeightForCoordinate(const int32 InX, const int32 InY) const
{
	 const float HeightRange = MaxHeight - MinHeight;
	// ==========================================================
    //            1. 대륙의 기반이 되는 저주파 노이즈 생성
    // ==========================================================
    const float ContinentNoiseInputX = InX * ContinentNoiseScale + NoiseOffset.X;
    const float ContinentNoiseInputY = InY * ContinentNoiseScale + NoiseOffset.Y;
    // ContinentNoise는 0~1 사이의 값으로, 평야(0)와 산맥 기반(1)을 나눈다.
    const float ContinentNoise = FMath::PerlinNoise2D(FVector2D(ContinentNoiseInputX, ContinentNoiseInputY)) * 0.5f + 0.5f;
    
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
        const float NoiseInputX = (InX * HeightNoiseScale * Frequency) + NoiseOffset.X;
        const float NoiseInputY = (InY * HeightNoiseScale * Frequency) + NoiseOffset.Y;
        const float PerlinValue = FMath::PerlinNoise2D(FVector2D(NoiseInputX, NoiseInputY)); // -1 ~ 1
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
    const float CombinedNoise = TerrainNoise * FMath::Pow(ContinentNoise, ContinentInfluence);
    
    // 최종 노이즈 값을 0~1 범위로 변환
    const float FinalNoiseValue = CombinedNoise * 0.5f + 0.5f;
    
    // 최종 높이 계산
    float Height = FinalNoiseValue;

    if (RedistributionFactor > 1.f && Height > 0.f && Height < 1.f)
    {
        const float PowX = FMath::Pow(Height, RedistributionFactor);
        const float Pow1_X = FMath::Pow(1-Height, RedistributionFactor);
        Height = PowX/(PowX + Pow1_X);
    }
    
    Height = FMath::Clamp(Height, 0.0f, 1.0f);
    
    // ==========================================================
    //         4. 최종 월드 높이로 변환
    // ==========================================================

    const float FinalWorldHeight = MinHeight + (Height * HeightRange);
    
    return FinalWorldHeight;
}

void UOCGLandscapeGenerateComponent::GenerateLandscape()
{
#if WITH_EDITOR
    // 랜드스케이프 생성
    const int32 QuadsPerSection = Landscape_QuadsPerSection;
    const int32 ComponentCountX = (LandscapeSize - 1) / (Landscape_QuadsPerSection * Landscape_SectionsPerComponent);
    const int32 ComponentCountY = (LandscapeSize - 1) / (Landscape_QuadsPerSection * Landscape_SectionsPerComponent);
    const int32 QuadsPerComponent = Landscape_SectionsPerComponent * QuadsPerSection;
    
    const int32 SizeX = ComponentCountX * QuadsPerComponent + 1;
    const int32 SizeY = ComponentCountY * QuadsPerComponent + 1;
    const int32 NumPixels = SizeX * SizeY;
	
    
    // 1. 하이트맵 데이터 배열 생성 (16비트)
    TArray<uint16> HeightData;
    HeightData.AddUninitialized(NumPixels);
    
    // 2. 하이트맵 데이터 채우기
    for (int32 y = 0; y < SizeY; ++y)
    {
        for (int32 x = 0; x < SizeX; ++x)
        {
            float CalculatedHeight = CalculateHeightForCoordinate(x, y);
            const float NormalizedHeight = 32768.0f + CalculatedHeight;
            const uint16 HeightValue = FMath::Clamp(FMath::RoundToInt(NormalizedHeight), 0, 65535);
            HeightData[y * SizeX + x] = HeightValue;
        }
    }
    ExportMap(HeightData, "HeightMap.png");

    TArray<uint16> TempData;
    GenerateTempMap(HeightData, TempData);

    TArray<uint16> HumidityData;
    GenerateHumidityMap(HeightData, TempData, HumidityData);

    DecideBiome(HeightData, TempData, HumidityData);

    // 3. 랜드스케이프 생성
    UWorld* World = GetWorld(); // 액터이므로 GetWorld()로 월드를 가져옵니다.
    if (!World || !World->IsEditorWorld()) // 에디터에서만 실행되도록 확인
    {
        UE_LOG(LogTemp, Error, TEXT("유효한 에디터 월드가 아닙니다."));
        return;
    }

    // 기존 랜드스케이프가 있다면 제거 (옵션)
    // for (TActorIterator<ALandscape> It(World); It; ++It)
    // {
    //     World->DestroyActor(*It);
    // }
    // 1) Landscape Actor 생성
    if (TargetLandscape)
    {
        TargetLandscape->Destroy();
    }
    
    if ((LandscapeSize - 1) % (Landscape_QuadsPerSection * Landscape_SectionsPerComponent) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("LandscapeSize is not a recommended value."));
    }

    // 빈 랜드스케이프 생성
    TargetLandscape = World->SpawnActor<ALandscape>();
    if (!TargetLandscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn ALandscape actor."));
        return;
    }
    TargetLandscape->bCanHaveLayersContent = true;
    TargetLandscape->LandscapeMaterial = MaterialInstance;
    
    // Import 함수에 전달할 하이트맵 데이터를 TMap 형태로 포장
    // 키(Key)는 레이어의 고유 ID(GUID)이고, 값(Value)은 해당 레이어의 하이트맵 데이터입니다.
    // 우리는 기본 레이어 하나만 있으므로, 하나만 만들어줍니다.
    
    TMap<FGuid, TArray<uint16>> HeightmapDataPerLayer;
    FGuid LayerGuid = FGuid();
    HeightmapDataPerLayer.Add(LayerGuid, MoveTemp(HeightData)); // MoveTemp로 소유권 이전

    // 2. 머티리얼 레이어 정보 TMap에도 동일한 GUID로 빈 데이터를 추가
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
    TArray<FLandscapeImportLayerInfo> ImportLayerDataPerLayer;
    const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
    ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();
    
    for (const auto& Biome : Biomes)
    {
        FLandscapeImportLayerInfo LayerInfo;
        LayerInfo.LayerName = Biome.BiomeName;
        LayerInfo.LayerData = Biome.WeightLayer;
        
        ULandscapeLayerInfoObject * LayerInfoObject = TargetLandscape->CreateLayerInfo(*LayerInfo.LayerName.ToString(), DefaultLayerInfo);
        if (LayerInfoObject != nullptr)
        {
            LayerInfoObject->LayerUsageDebugColor = LayerInfoObject->GenerateLayerUsageDebugColor();
            LayerInfoObject->MarkPackageDirty();
        }
        LayerInfo.LayerInfo = LayerInfoObject;
        ImportLayerDataPerLayer.Add(LayerInfo);
    }
    MaterialLayerDataPerLayer.Add(LayerGuid, ImportLayerDataPerLayer);
    
    // 트랜잭션 시작 (Undo/Redo를 위함)
    FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "LandscapeEditor_CreateLandscape", "Create Landscape"));

    // 랜드스케이프의 기본 속성 설정
    float Offset = (-LandscapeSize/2.f) * 100.f;
    float HeightOffset = -(MinHeight) / 2.f;
    TargetLandscape->SetActorLocation(FVector(Offset, Offset, 0));
    TargetLandscape->SetActorScale3D(FVector(100.0f, 100.0f, 100.0f));
    
    // Import 함수 호출 (변경된 시그니처에 맞춰서)
    TargetLandscape->Import(
        FGuid::NewGuid(),
        0, 0,
        LandscapeSize - 1, LandscapeSize - 1,
        Landscape_SectionsPerComponent,
        QuadsPerSection,
        HeightmapDataPerLayer,
        nullptr,
        MaterialLayerDataPerLayer,
        ELandscapeImportAlphamapType::Additive,
        TArrayView<const FLandscapeLayer>() // 빈 TArray로부터 TArrayView 생성하여 전달
    );

    FinalizeLayerInfos(TargetLandscape, MaterialLayerDataPerLayer);
    
    // 액터 등록 및 뷰포트 업데이트
    TargetLandscape->RegisterAllComponents();
    GEditor->RedrawAllViewports();
#endif
}

void UOCGLandscapeGenerateComponent::GenerateTempMap(const TArray<uint16>& InHeightMap, TArray<uint16>& OutTempMap)
{
	if (OutTempMap.Num() != LandscapeSize * LandscapeSize)
    {
        OutTempMap.SetNumUninitialized(LandscapeSize * LandscapeSize);
    }
    
    TArray<float> TempMapFloat;
    TempMapFloat.SetNumUninitialized(LandscapeSize * LandscapeSize);

    float GlobalMinTemp = TNumericLimits<float>::Max();
    float GlobalMaxTemp = TNumericLimits<float>::Lowest();
    float TempRange = MaxTemp - MinTemp;

    for (int32 y = 0; y < LandscapeSize; ++y)
    {
        for (int32 x = 0; x < LandscapeSize; ++x)
        {
            const int32 Index = y * LandscapeSize + x;
            
            // ==========================================================
            //       ★ 1. 노이즈 기반으로 기본 온도 설정 ★
            // ==========================================================
            // 매우 낮은 주파수의 노이즈를 사용하여 따뜻한 지역과 추운 지역의 큰 덩어리를 만듭니다.
            const float TempNoiseInputX = x * 0.002f + NoiseOffset.X;
            const float TempNoiseInputY = y * 0.002f + NoiseOffset.Y;

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
            const float FinalTemp = FMath::Clamp(BaseTemp, MinTemp, MaxTemp);
            
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

void UOCGLandscapeGenerateComponent::GenerateHumidityMap(const TArray<uint16>& InHeightMap,
	const TArray<uint16>& InTempMap, TArray<uint16>& OutHumidityMap)
{
    if (OutHumidityMap.Num() != LandscapeSize * LandscapeSize)
    {
        OutHumidityMap.SetNumUninitialized(LandscapeSize * LandscapeSize);
    }
    
    // --- 사전 준비 ---
    const float SeaLevelWorldHeight = MinHeight + SeaLevel * (MaxHeight - MinHeight);

    // 임시 float 배열들을 사용하여 중간 계산 결과를 저장합니다.
    TArray<float> HumidityMapFloat;
    HumidityMapFloat.Init(0.0f, LandscapeSize * LandscapeSize);

    // 각 픽셀에서 가장 가까운 물까지의 거리를 저장할 배열
    TArray<float> DistanceToWater;
    DistanceToWater.Init(TNumericLimits<float>::Max(), LandscapeSize * LandscapeSize);
    
    // ==========================================================
    //            Pass 1: 물 공급원 찾기 및 거리 계산
    // ==========================================================
    
    // 1-A: 모든 물 픽셀(습기 공급원)을 찾아서 큐에 넣습니다.
    TQueue<FIntPoint> Frontier; // 너비 우선 탐색(BFS)을 위한 큐
    for (int32 y = 0; y < LandscapeSize; ++y)
    {
        for (int32 x = 0; x < LandscapeSize; ++x)
        {
            const int32 Index = y * LandscapeSize + x;
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
            
            if (nx >= 0 && nx < LandscapeSize && ny >= 0 && ny < LandscapeSize)
            {
                int32 NeighborIndex = ny * LandscapeSize + nx;
                // 아직 방문하지 않은(거리가 무한대인) 이웃 픽셀이라면
                if (DistanceToWater[NeighborIndex] == TNumericLimits<float>::Max())
                {
                    DistanceToWater[NeighborIndex] = DistanceToWater[Current.Y * LandscapeSize + Current.X] + 1.0f;
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
    
    for (int32 i = 0; i < LandscapeSize * LandscapeSize; ++i)
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
            const float HumidityFromDistance = FMath::Exp(-DistanceToWater[i] * MoistureFalloffRate);
        
            // 2-B: 온도의 영향을 적용합니다.
            const float NormalizedTemp = static_cast<float>(InTempMap[i]) / 65535.0f;
            FinalHumidity = HumidityFromDistance * (1.0f - (NormalizedTemp * TemperatureInfluenceOnHumidity));
        }

        FinalHumidity = FMath::Clamp(FinalHumidity, 0.0f, 1.0f);
        
        if (RedistributionFactor > 1.f && FinalHumidity > 0.f && FinalHumidity < 1.f)
        {
            const float PowX = FMath::Pow(FinalHumidity, RedistributionFactor);
            const float Pow1_X = FMath::Pow(1-FinalHumidity, RedistributionFactor);
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

void UOCGLandscapeGenerateComponent::ExportMap(const TArray<uint16>& InMap, const FString& FileName) const
{
#if WITH_EDITOR
    // 1. 저장할 파일 경로를 동적으로 생성합니다.
    // 프로젝트의 콘텐츠 폴더 경로를 가져옵니다. (예: "C:/MyProject/Content/")
    const FString ContentDir = FPaths::ProjectContentDir();
    
    // 하위 폴더와 파일 이름을 지정합니다. 시드 값을 파일 이름에 포함시켜 구별하기 쉽게 만듭니다.
    const FString SubDir = TEXT("Maps/");
    
    // 최종 전체 파일 경로를 조합합니다.
    const FString FullPath = FPaths::Combine(ContentDir, SubDir, FileName);

    // 디렉토리가 존재하지 않으면 생성합니다.
    const FString DirectoryPath = FPaths::GetPath(FullPath);
    if (!FPaths::DirectoryExists(DirectoryPath))
    {
        IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
        if (!PlatformFile.CreateDirectoryTree(*DirectoryPath))
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create directory: %s"), *DirectoryPath);
            return;
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Attempting to export heightmap to: %s"), *FullPath);

    // 2. 이미지 래퍼(Image Wrapper) 모듈을 로드합니다.
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    
    // PNG 형식으로 저장할 래퍼를 생성합니다.
    const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper."));
        return;
    }

    // 3. 이미지 래퍼에 데이터 설정
    // 16비트 그레이스케일(G16) 형식으로 데이터를 설정합니다.
    if (ImageWrapper->SetRaw(InMap.GetData(), InMap.GetAllocatedSize(), LandscapeSize, LandscapeSize, ERGBFormat::Gray, 16))
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

void UOCGLandscapeGenerateComponent::DecideBiome(const TArray<uint16>& InHeightMap, const TArray<uint16>& InTempMap,
	const TArray<uint16>& InHumidityMap)
{
    TArray<FColor> BiomeColorMap;
    BiomeColorMap.AddUninitialized(LandscapeSize * LandscapeSize);
    
    for (auto& Biome : Biomes)
    {
        Biome.WeightLayer.Init(0, LandscapeSize * LandscapeSize);
    }

    const float SeaLevelHeight = MinHeight + SeaLevel * (MaxHeight - MinHeight);
    
    for (int32 y = 0; y < LandscapeSize; ++y)
    {
        for (int32 x = 0; x < LandscapeSize; ++x)
        {
            const int32 Index = y * LandscapeSize + x;
            float Height = static_cast<float>(InHeightMap[Index]);
            Height -= 32768.f;
            float NormalizedTemp = static_cast<float>(InTempMap[y*LandscapeSize+x]) / 65535.f;
            const float Temp = FMath::Lerp(CachedGlobalMinTemp, CachedGlobalMaxTemp, NormalizedTemp);
            float NormalizedHumidity = static_cast<float>(InHumidityMap[y*LandscapeSize+x]) / 65535.f;
            const float Humidity = FMath::Lerp(CachedGlobalMinHumidity, CachedGlobalMaxHumidity, NormalizedHumidity);
            if (y == LandscapeSize / 2 && x == LandscapeSize / 2)
                UE_LOG(LogTemp, Display, TEXT("Temp : %f, Humidity : %f"), Temp, Humidity);
            FOCGBiomeSettings* CurrentBiome = nullptr;
            if (Height <= SeaLevelHeight)
            {
                CurrentBiome = FindBiome(TEXT("Water"));
            }
            else
            {
                float MinDist = TNumericLimits<float>::Max();
                for (auto& Biome : Biomes)
                {
                    if (Biome.BiomeName == TEXT("Water"))
                        continue;
                    const float TempDiff = FMath::Abs(Biome.Temperature - Temp);
                    float HumidityDiff = FMath::Abs(Biome.Humidity - Humidity);
                    HumidityDiff = (MaxTemp - MinTemp) * HumidityDiff;
                    const float Dist = FVector2D(TempDiff, HumidityDiff).Length();
                    if (Dist < MinDist)
                    {
                        MinDist = Dist;
                        CurrentBiome = &Biome;
                    }
                }
            }
            if(CurrentBiome)
            {
                CurrentBiome->WeightLayer[Index] = 255;
                BiomeColorMap[Index] = CurrentBiome->Color.ToFColor(true);
            }
        }
    }
    ExportBiomeMap(BiomeColorMap, "BiomeMap.png");
}

void UOCGLandscapeGenerateComponent::ExportBiomeMap(const TArray<FColor>& InBiomeMap, const FString FileName)
{
    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    if (!ImageWrapper.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create Image Wrapper for color map."));
        return;
    }

    // BGRA8 형식으로 원본 데이터를 설정합니다. FColor는 내부적으로 BGRA 순서입니다.
    if (ImageWrapper->SetRaw(InBiomeMap.GetData(), InBiomeMap.GetAllocatedSize(), LandscapeSize, LandscapeSize, ERGBFormat::BGRA, 8))
    {
        const TArray64<uint8>& PngData = ImageWrapper->GetCompressed(100);
        
        const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps"), FileName);
        // 디렉토리 생성 로직 (필요 시 추가)
        // ...

        if (FFileHelper::SaveArrayToFile(PngData, *FullPath))
        {
            UE_LOG(LogTemp, Log, TEXT("Color map exported successfully to: %s"), *FullPath);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to save color map file."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to set raw color data to Image Wrapper."));
    }
}

void UOCGLandscapeGenerateComponent::FinalizeLayerInfos(ALandscape* Landscape,
	const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers)
{
#if WITH_EDITOR
    if (!Landscape) return;
    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    if (LandscapeInfo)
    {
        LandscapeInfo->UpdateLayerInfoMap(Landscape);
    }

    // Assume single entry in map
    const TArray<FLandscapeImportLayerInfo>& ImportInfos = MaterialLayerDataPerLayers.begin()->Value;
    const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
    const ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();

    for (const FLandscapeImportLayerInfo& ImportInfo : ImportInfos)
    {
        ULandscapeLayerInfoObject* LayerInfoObj = ImportInfo.LayerInfo;
        const FName LayerName = ImportInfo.LayerName;
        if (!LayerInfoObj)
        {
            LayerInfoObj = Landscape->CreateLayerInfo(*LayerName.ToString(), DefaultLayerInfo);
            if (LayerInfoObj)
            {
                LayerInfoObj->LayerUsageDebugColor = LayerInfoObj->GenerateLayerUsageDebugColor();
                LayerInfoObj->MarkPackageDirty();
            }
        }
		
        if (LayerInfoObj)
        {
            Landscape->AddTargetLayer(LayerName, FLandscapeTargetLayerSettings(LayerInfoObj));
            if (LandscapeInfo)
            {
                const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerName);
                if (Index != INDEX_NONE)
                {
                    LandscapeInfo->Layers[Index].LayerInfoObj = LayerInfoObj;
                }
            }
        }
    }
#endif
}

FOCGBiomeSettings* UOCGLandscapeGenerateComponent::FindBiome(const FName BiomeName)
{
    for (auto& Biome : Biomes)
    {
        if (Biome.BiomeName == BiomeName)
            return &Biome;
    }
    
    return nullptr;
}

