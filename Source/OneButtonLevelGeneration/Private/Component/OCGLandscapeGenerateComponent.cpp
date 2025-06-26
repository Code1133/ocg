// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/OCGLandscapeGenerateComponent.h"
#include "EngineUtils.h"
#include "OCGLevelGenerator.h"
#include "OCGMaterialEditTool.h"
#include "VisualizeTexture.h"
#include "Components/SkyLightComponent.h"
#include "Data/MapPreset.h"
#include "Data/OCGBiomeSettings.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"

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

    // 2. 머티리얼 레이어 정보 TMap에도 동일한 GUID로 빈 데이터를 추가
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer;
    TArray<FLandscapeImportLayerInfo> ImportLayerDataPerLayer;
    const ULandscapeSettings* Settings = GetDefault<ULandscapeSettings>();
    ULandscapeLayerInfoObject* DefaultLayerInfo = Settings->GetDefaultLayerInfoObject().LoadSynchronous();

    TMap<FName, TArray<uint8>> WeightLayers = LevelGenerator->GetWeightLayers();
    TArray<FName> LayerNames = OCGMaterialEditTool::ExtractLandscapeLayerName(Cast<UMaterial>(MapPreset->LandscapeMaterial->Parent));
    
    for (int32 Index = 0; Index < LevelGenerator->GetMapPreset()->Biomes.Num(); ++Index)
    {
        FLandscapeImportLayerInfo LayerInfo;
        
        FString LayerNameStr = FString::Printf(TEXT("Layer%d"), Index);
        FName LayerName(LayerNameStr);
        
        LayerInfo.LayerData = WeightLayers.FindChecked(LayerName);
        LayerInfo.LayerName = LayerNames[Index];
        
        //LayerInfo.LayerName = Biome.BiomeName;
        //LayerInfo.LayerData = WeightLayers.FindChecked(Biome.BiomeName);

        // TODO : Material의 Layer의 이름도 맞춰야 함
        
        ULandscapeLayerInfoObject * LayerInfoObject = TargetLandscape->CreateLayerInfo(*LayerInfo.LayerName.ToString(), DefaultLayerInfo);
        if (LayerInfoObject != nullptr)
        {
            LayerInfoObject->LayerUsageDebugColor = LayerInfoObject->GenerateLayerUsageDebugColor();
            LayerInfoObject->MarkPackageDirty();
        }
        LayerInfo.LayerInfo = LayerInfoObject;
        ImportLayerDataPerLayer.Add(LayerInfo);
    }
    // for (const auto& Biome : LevelGenerator->GetMapPreset()->Biomes)
    // {
    //     FLandscapeImportLayerInfo LayerInfo;
    //     LayerInfo.LayerName = Biome.BiomeName;
    //     LayerInfo.LayerData = WeightLayers.FindChecked(Biome.BiomeName);
    //
    //     // TODO : Material의 Layer의 이름도 맞춰야 함
    //     
    //     ULandscapeLayerInfoObject * LayerInfoObject = TargetLandscape->CreateLayerInfo(*LayerInfo.LayerName.ToString(), DefaultLayerInfo);
    //     if (LayerInfoObject != nullptr)
    //     {
    //         LayerInfoObject->LayerUsageDebugColor = LayerInfoObject->GenerateLayerUsageDebugColor();
    //         LayerInfoObject->MarkPackageDirty();
    //     }
    //     LayerInfo.LayerInfo = LayerInfoObject;
    //     ImportLayerDataPerLayer.Add(LayerInfo);
    // }
    MaterialLayerDataPerLayer.Add(LayerGuid, ImportLayerDataPerLayer);
    
    // 트랜잭션 시작 (Undo/Redo를 위함)
    FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "LandscapeEditor_CreateLandscape", "Create Landscape"));

    // 랜드스케이프의 기본 속성 설정
    float OffsetX = (-MapPreset->MapResolution.X / 2.f) * 100.f;
    float OffsetY = (-MapPreset->MapResolution.Y / 2.f) * 100.f;
    TargetLandscape->SetActorLocation(FVector(OffsetX, OffsetY, 0));
    TargetLandscape->SetActorScale3D(FVector(100.0f, 100.0f, 100.0f));
    
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

AOCGLevelGenerator* UOCGLandscapeGenerateComponent::GetLevelGenerator() const
{
    return Cast<AOCGLevelGenerator>(GetOwner());
}
