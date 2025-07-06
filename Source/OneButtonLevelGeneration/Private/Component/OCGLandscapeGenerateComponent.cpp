// Copyright (c) 2025 Code1133. All rights reserved.

#include "Component/OCGLandscapeGenerateComponent.h"
#include "EngineUtils.h"
#include "ObjectTools.h"
#include "OCGLevelGenerator.h"
#include "VisualizeTexture.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SkyLightComponent.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"

#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureVolume.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "RuntimeVirtualTextureSetBounds.h"
#include "Component/OCGMapGenerateComponent.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeSettings.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Util/OCGFileUtils.h"
#include "Util/OCGMaterialEditTool.h"
#endif

static void GetMissingRuntimeVirtualTextureVolumes(ALandscape* InLandscapeActor, TArray<URuntimeVirtualTexture*>& OutVirtualTextures)
{
    UWorld* World = InLandscapeActor != nullptr ? InLandscapeActor->GetWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    TArray<URuntimeVirtualTexture*> FoundVolumes;
    for (TObjectIterator<URuntimeVirtualTextureComponent> It(RF_ClassDefaultObject, false, EInternalObjectFlags::Garbage); It; ++It)
    {
        if (It->GetWorld() == World)
        {
            if (URuntimeVirtualTexture* VirtualTexture = It->GetVirtualTexture())
            {
                FoundVolumes.Add(VirtualTexture);
            }
        }
    }

    for (URuntimeVirtualTexture* VirtualTexture : InLandscapeActor->RuntimeVirtualTextures)
    {
        if (VirtualTexture != nullptr && FoundVolumes.Find(VirtualTexture) == INDEX_NONE)
        {
            OutVirtualTextures.Add(VirtualTexture);
        }
    }
}

// Sets default values for this component's properties
UOCGLandscapeGenerateComponent::UOCGLandscapeGenerateComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
    static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> ColorRVTFinder(TEXT("/Script/Engine.RuntimeVirtualTexture'/OneButtonLevelGeneration/RVT/RVT_Color.RVT_Color'"));
    if (ColorRVTFinder.Succeeded())
    {
        ColorRVT = ColorRVTFinder.Object;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find default Color RuntimeVirtualTexture at the specified path."));
    }

    static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> HeightRVTFinder(TEXT("/Script/Engine.RuntimeVirtualTexture'/OneButtonLevelGeneration/RVT/RVT_Height.RVT_Height'"));
    if (HeightRVTFinder.Succeeded())
    {
        HeightRVT = HeightRVTFinder.Object;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find default Height RuntimeVirtualTexture at the specified path."));
    }

    static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> DisplacementRVTFinder(TEXT("/Script/Engine.RuntimeVirtualTexture'/OneButtonLevelGeneration/RVT/RVT_Displacement.RVT_Displacement'"));
    if (DisplacementRVTFinder.Succeeded())
    {
        DisplacementRVT = DisplacementRVTFinder.Object;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to find default Displacement RuntimeVirtualTexture at the specified path."));
    }
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

void UOCGLandscapeGenerateComponent::GenerateLandscapeInEditor()
{
    GenerateLandscape(GetWorld());
}

void UOCGLandscapeGenerateComponent::GenerateLandscape(UWorld* World)
{
#if WITH_EDITOR
    AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    FIntPoint MapResolution = MapPreset->MapResolution;
    
    // 랜드스케이프 생성
    const int32 QuadsPerSection = static_cast<uint32>(MapPreset->Landscape_QuadsPerSection);
    const int32 ComponentCountX = (MapResolution.X - 1) / (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent);
    const int32 ComponentCountY = (MapResolution.Y - 1) / (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent);
    const int32 QuadsPerComponent = MapPreset->Landscape_SectionsPerComponent * QuadsPerSection;
    
    const int32 SizeX = ComponentCountX * QuadsPerComponent + 1;
    const int32 SizeY = ComponentCountY * QuadsPerComponent + 1;
    const int32 NumPixels = SizeX * SizeY;

    if (!World || World->IsGameWorld()) // 에디터에서만 실행되도록 확인
    {
        UE_LOG(LogTemp, Error, TEXT("유효한 에디터 월드가 아닙니다."));
        return;
    }
    
    if (TargetLandscape)
    {
        TargetLandscape->Destroy();
    }
    
    if ((MapResolution.X - 1) % (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent) != 0 || (MapResolution.Y - 1) % (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("LandscapeSize is not a recommended value."));
    }
    
    TargetLandscape = World->SpawnActor<ALandscape>();
    if (!TargetLandscape)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to spawn ALandscape actor."));
        return;
    }
    TargetLandscape->bCanHaveLayersContent = true;
    TargetLandscape->bCanHaveLayersContent = true;
    TargetLandscape->LandscapeMaterial = MapPreset->LandscapeMaterial;
    
    // Import 함수에 전달할 하이트맵 데이터를 TMap 형태로 포장
    // 키(Key)는 레이어의 고유 ID(GUID)이고, 값(Value)은 해당 레이어의 하이트맵 데이터입니다.
    // 우리는 기본 레이어 하나만 있으므로, 하나만 만들어줍니다.
    
    TMap<FGuid, TArray<uint16>> HeightmapDataPerLayer;
    FGuid LayerGuid = FGuid();
    HeightmapDataPerLayer.Add(LayerGuid, LevelGenerator->GetHeightMapData());

    // 레이어 데이터 준비
    const TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer = PrepareLandscapeLayerData(TargetLandscape, LevelGenerator, MapPreset);

    // 트랜잭션 시작 (Undo/Redo를 위함)
    FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "LandscapeEditor_CreateLandscape", "Create Landscape"));

    // 랜드스케이프의 기본 속성 설정
    float OffsetX = (-MapPreset->MapResolution.X / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetY = (-MapPreset->MapResolution.Y / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetZ = (MapPreset->MaxHeight - MapPreset->MinHeight) / 2.f;
    TargetLandscape->SetActorLocation(FVector(OffsetX, OffsetY, 0));
    TargetLandscape->SetActorScale3D(FVector(100.0f, 100.0f, 100.0f) * MapPreset->LandscapeScale);
    
    // Import 함수 호출 (변경된 시그니처에 맞춰서)
    TargetLandscape->Import(
        FGuid::NewGuid(),
        0, 0,
        MapResolution.X - 1, MapResolution.Y - 1,
        MapPreset->Landscape_SectionsPerComponent,
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

    TargetLandscape->ForceUpdateLayersContent(true);

    TargetLandscape->RuntimeVirtualTextures.Add(ColorRVT);
    TargetLandscape->RuntimeVirtualTextures.Add(HeightRVT);
    TargetLandscape->RuntimeVirtualTextures.Add(DisplacementRVT);
    
    CreateRuntimeVirtualTextureVolume(TargetLandscape);
    CachePointsForRiverGeneration();
#endif
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

TMap<FGuid, TArray<FLandscapeImportLayerInfo>> UOCGLandscapeGenerateComponent::PrepareLandscapeLayerData(
    ALandscape* InTargetLandscape, AOCGLevelGenerator* InLevelGenerator, const UMapPreset* InMapPreset)
{
#if WITH_EDITOR
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
    if (!InTargetLandscape || !InLevelGenerator || !InMapPreset)
    {
        return MaterialLayerDataPerLayer; // 유효하지 않은 입력이면 빈 맵 반환
    }

    ULandscapeInfo* LandscapeInfo = InTargetLandscape->GetLandscapeInfo();

    TArray<FLandscapeImportLayerInfo> ImportLayerDataPerLayer;
    const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
    ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();

    // 1. 웨이트맵 데이터와 머티리얼에서 레이어 이름 가져오기
    TMap<FName, TArray<uint8>> WeightLayers = InLevelGenerator->GetWeightLayers();
    TArray<FName> LayerNames;
    if (InMapPreset->LandscapeMaterial && InMapPreset->LandscapeMaterial->Parent)
    {
        LayerNames = OCGMaterialEditTool::ExtractLandscapeLayerName(Cast<UMaterial>(InMapPreset->LandscapeMaterial->Parent));
    }

    // 2. 각 레이어를 순회하며 LayerInfoObject를 찾거나 생성
    for (int32 Index = 0; Index < WeightLayers.Num(); ++Index)
    {
        FLandscapeImportLayerInfo LayerInfo;
        
        // 임시 이름 생성 (머티리얼에서 이름을 못 찾을 경우 대비)
        FString TempLayerNameStr = FString::Printf(TEXT("Layer%d"), Index);
        FName TempLayerName(TempLayerNameStr);
        
        LayerInfo.LayerData = WeightLayers.FindChecked(TempLayerName);
        
        if (LayerNames.IsValidIndex(Index))
        {
            LayerInfo.LayerName = LayerNames[Index];
        }
        else
        {
            LayerInfo.LayerName = TempLayerName;
            UE_LOG(LogTemp, Warning, TEXT("Layer %d not found in Material Names, using default name: %s"), Index, *TempLayerName.ToString());
        }

        ULandscapeLayerInfoObject* LayerInfoObject = nullptr;
        if (LandscapeInfo)
        {
            LayerInfoObject = LandscapeInfo->GetLayerInfoByName(LayerInfo.LayerName);
        }
        
        if (LayerInfoObject == nullptr)
        {
            UE_LOG(LogTemp, Log, TEXT("LayerInfo for '%s' not found. Creating a new one."), *LayerInfo.LayerName.ToString());
            
            // LayerInfoSavePath는 멤버 변수로 가정합니다. 필요시 파라미터로 전달할 수 있습니다.
            LayerInfoObject = CreateLayerInfo(InTargetLandscape, LayerInfoSavePath, LayerInfo.LayerName.ToString(), DefaultLayerInfo);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Found and reused existing LayerInfo for '%s'."), *LayerInfo.LayerName.ToString());
        }
        
        if(LayerInfoObject)
        {
            LayerInfo.LayerInfo = LayerInfoObject;
            ImportLayerDataPerLayer.Add(LayerInfo);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to find or create LayerInfo for '%s'."), *LayerInfo.LayerName.ToString());
        }
    }

    // 3. 최종 데이터 구조에 추가하여 반환
    FGuid LayerGuid = FGuid(); // 기본 레이어 세트의 GUID
    MaterialLayerDataPerLayer.Add(LayerGuid, ImportLayerDataPerLayer);
    
    return MaterialLayerDataPerLayer;
#endif
}

ULandscapeLayerInfoObject* UOCGLandscapeGenerateComponent::CreateLayerInfo(ALandscape* InLandscape,
                                                                           const FString& InPackagePath, const FString& InAssetName, const ULandscapeLayerInfoObject* InTemplate)
{
#if WITH_EDITOR
    if (!InLandscape)
    {
        UE_LOG(LogTemp, Error, TEXT("TargetLandscape is null."));
        return nullptr;
    }

    // 위에서 정의한 함수를 사용하여 지정된 경로에 LayerInfo 애셋을 생성합니다.
    ULandscapeLayerInfoObject* LayerInfo = CreateLayerInfo(InPackagePath, InAssetName, InTemplate);

    if (LayerInfo)
    {
        ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
        if (LandscapeInfo)
        {
            // 랜드스케이프 정보에 새로 생성된 LayerInfo를 추가하거나 업데이트합니다.
            // 이 과정은 랜드스케이프가 해당 레이어를 인식하게 만듭니다.
            const int32 Index = LandscapeInfo->GetLayerInfoIndex(LayerInfo->LayerName, InLandscape);
            if (Index == INDEX_NONE)
            {
                LandscapeInfo->Layers.Add(FLandscapeInfoLayerSettings(LayerInfo, InLandscape));
            }
            else
            {
                LandscapeInfo->Layers[Index].LayerInfoObj = LayerInfo;
            }
        }
    }

    return LayerInfo;
#endif
}

ULandscapeLayerInfoObject* UOCGLandscapeGenerateComponent::CreateLayerInfo(const FString& InPackagePath, const FString& InAssetName,
                                                                           const ULandscapeLayerInfoObject* InTemplate)
{
#if WITH_EDITOR
    // 1. 경로 유효성 검사
    if (!InPackagePath.StartsWith(TEXT("/Game/")))
    {
        UE_LOG(LogTemp, Error, TEXT("PackagePath must start with /Game/. Path was: %s"), *InPackagePath);
        return nullptr;
    }
    
    // 2. 가장 먼저, 입력된 애셋 이름을 안전한 이름으로 변환합니다.
    // 이 정리된 이름이 앞으로 모든 작업의 기준이 됩니다.
    const FString SanitizedAssetName = ObjectTools::SanitizeObjectName(InAssetName);
    if (SanitizedAssetName.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("Asset name '%s' became empty after sanitization."), *InAssetName);
        return nullptr;
    }

    // 3. 안전한 이름을 사용하여 전체 패키지 경로와 오브젝트 경로를 구성합니다.
    const FString FullPackageName = FPaths::Combine(InPackagePath, SanitizedAssetName);
    const FString ObjectPathToLoad = FullPackageName + TEXT(".") + SanitizedAssetName;

    // 4. 안전한 경로를 사용하여 애셋이 이미 존재하는지 로드를 시도합니다.
    ULandscapeLayerInfoObject* FoundLayerInfo = LoadObject<ULandscapeLayerInfoObject>(nullptr, *ObjectPathToLoad);
    
    // 5. 애셋을 찾았다면, 즉시 반환합니다.
    if (FoundLayerInfo)
    {
        UE_LOG(LogTemp, Log, TEXT("Found and reused existing LayerInfo: %s"), *ObjectPathToLoad);
        return FoundLayerInfo;
    }

    // 애셋을 찾지 못했으므로, 새로 생성하기 전에 디렉토리 존재를 확인하고 필요시 생성합니다.
    // InPackagePath는 "/Game/MyFolder/MySubFolder" 형태일 것으로 예상됩니다.
    if (!FOCGFileUtils::EnsureContentDirectoryExists(FullPackageName))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to ensure directory exists for package path: %s"), *InPackagePath);
        return nullptr;
    }

    // 6. 애셋을 찾지 못했으므로, 새로 생성하는 로직을 실행합니다.
    UE_LOG(LogTemp, Log, TEXT("LayerInfo not found at '%s'. Creating a new one."), *ObjectPathToLoad);

    const FName AssetFName(*SanitizedAssetName);

    UPackage* Package = CreatePackage(*FullPackageName);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create package: %s"), *FullPackageName);
        return nullptr;
    }
    
    ULandscapeLayerInfoObject* NewLayerInfo = nullptr;
    if (InTemplate)
    {
        NewLayerInfo = DuplicateObject<ULandscapeLayerInfoObject>(InTemplate, Package, AssetFName);
    }
    else
    {
        NewLayerInfo = NewObject<ULandscapeLayerInfoObject>(Package, AssetFName, RF_Public | RF_Standalone | RF_Transactional);
    }

    if (!NewLayerInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create ULandscapeLayerInfoObject in package: %s"), *FullPackageName);
        return nullptr;
    }
    
    // LayerName 프로퍼티에도 안전한 이름을 할당합니다.
    NewLayerInfo->LayerName = AssetFName;
    
    FAssetRegistryModule::AssetCreated(NewLayerInfo);
    NewLayerInfo->LayerUsageDebugColor = NewLayerInfo->GenerateLayerUsageDebugColor();
    NewLayerInfo->MarkPackageDirty();

    return NewLayerInfo;
#endif
}

AOCGLevelGenerator* UOCGLandscapeGenerateComponent::GetLevelGenerator() const
{
    return Cast<AOCGLevelGenerator>(GetOwner());
}

bool UOCGLandscapeGenerateComponent::CreateRuntimeVirtualTextureVolume(ALandscape* InLandscapeActor)
{
#if WITH_EDITOR
    if (InLandscapeActor == nullptr)
    {
        return false;
    }

    TArray<URuntimeVirtualTexture*> VirtualTextureVolumesToCreate;
    GetMissingRuntimeVirtualTextureVolumes(InLandscapeActor, VirtualTextureVolumesToCreate);
    if (VirtualTextureVolumesToCreate.Num() == 0)
    {
        return false;
    }
    
    for (URuntimeVirtualTexture* VirtualTexture : VirtualTextureVolumesToCreate)
    {
        ARuntimeVirtualTextureVolume* NewVolume = InLandscapeActor->GetWorld()->SpawnActor<ARuntimeVirtualTextureVolume>();
        NewVolume->VirtualTextureComponent->SetVirtualTexture(VirtualTexture);
        NewVolume->VirtualTextureComponent->SetBoundsAlignActor(InLandscapeActor);
        RuntimeVirtualTexture::SetBounds(NewVolume->VirtualTextureComponent);
        
        NewVolume->VirtualTextureComponent->SetBoundsAlignActor(nullptr);
    }

    return true;
#endif
}

FVector UOCGLandscapeGenerateComponent::GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const
{
    if (!TargetLandscape || !GetLevelGenerator())
    {
        UE_LOG(LogTemp , Error, TEXT("TargetLandscape or MapPreset is not set. Cannot get world position."));
        return FVector::ZeroVector;
    }

    const UMapPreset* MapPreset = GetLevelGenerator()->GetMapPreset();

    float OffsetX = (-MapPreset->MapResolution.X / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetY = (-MapPreset->MapResolution.Y / 2.f) * 100.f * MapPreset->LandscapeScale;
	
    FVector WorldLocation = LandscapeOrigin + FVector(
        2 * (MapPoint.X / (float)MapPreset->MapResolution.X) * LandscapeExtent.X + OffsetX,
        2 * (MapPoint.Y / (float)MapPreset->MapResolution.Y) * LandscapeExtent.Y + OffsetY,
        0.0f 
    );

    int Index = MapPoint.Y * MapPreset->MapResolution.X + MapPoint.X;
    float HeightMapValue = GetLevelGenerator()->GetHeightMapData()[Index];

    WorldLocation.Z  = (HeightMapValue - 32768) / 128 * 100 * MapPreset->LandscapeScale; // Adjust height based on the height map value and scale
	
    return WorldLocation;
}

void UOCGLandscapeGenerateComponent::CachePointsForRiverGeneration()
{
    if (!TargetLandscape || !GetLevelGenerator() || !GetLevelGenerator()->GetMapPreset())
    {
        UE_LOG(LogTemp, Error, TEXT("TargetLandscape or LevelGenerator is not set. Cannot cache river start points."));
        return;
    }

    if (GetLevelGenerator()->GetMapPreset()->bGenerateRiver == false)
    {
        return;
    }

    CachedRiverStartPoints.Empty();
    
    const UMapPreset* MapPreset = GetLevelGenerator()->GetMapPreset();
    
    // Ensure the multiplier is within a reasonable range
	float StartPointThresholdMultiplier = FMath::Clamp(MapPreset->RiverStartPointThresholdMultiplier, 0.0f, 1.0f);
    float MaxHeight = GetLevelGenerator()->GetMapGenerateComponent()->GetMaxHeight() * MapPreset->LandscapeScale;
    float MinHeight = GetLevelGenerator()->GetMapGenerateComponent()->GetMinHeight() * MapPreset->LandscapeScale;
    
    float SeaHeight = MinHeight + 
        (MaxHeight - MinHeight) * MapPreset->SeaLevel - 1;
    
    uint16 HighThreshold = SeaHeight + (MaxHeight - SeaHeight) * StartPointThresholdMultiplier;
    UE_LOG(LogTemp, Log, TEXT("High Threshold for River Start Point: %d"), HighThreshold);

    FBox LandscapeBounds = TargetLandscape->GetComponentsBoundingBox();
    FVector LandscapeOrigin = LandscapeBounds.GetCenter();
    FVector LandscapeExtent = LandscapeBounds.GetExtent();
    for (int32 y = 0; y < MapPreset->MapResolution.Y; ++y)
    {
        for (int32 x = 0; x < MapPreset->MapResolution.X; ++x)
        {
            FVector WorldLocation = GetLandscapePointWorldPosition(FIntPoint(x, y), LandscapeOrigin, LandscapeExtent);
            if (WorldLocation.Z >= HighThreshold)
            {
                CachedRiverStartPoints.Add(FIntPoint(x, y));
            }
        }
    }
}
