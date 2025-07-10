// Copyright (c) 2025 Code1133. All rights reserved.

#include "Component/OCGLandscapeGenerateComponent.h"
#include "EngineUtils.h"
#include "NaniteSceneProxy.h"

#include "ObjectTools.h"
#include "OCGLevelGenerator.h"
#include "PCGCommon.h"
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
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/ConstructorHelpers.h"
#include "WorldPartition/WorldPartitionHelpers.h"
#include "WorldPartition/WorldPartitionSubsystem.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeSettings.h"
#include "LandscapeSubsystem.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Util/OCGFileUtils.h"
#include "Util/OCGMaterialEditTool.h"

#include "LandscapeProxy.h"
#include "LandscapeSettings.h"
#include "LandscapeSubsystem.h"
#include "LocationVolume.h"
#include "Builders/CubeBuilder.h"
#include "LandscapeStreamingProxy.h"
#include "ActorFactories/ActorFactory.h"
#include "WorldPartition/WorldPartition.h"

#include "LandscapeConfigHelper.h"
#include "LandscapeEdit.h"
#include "LandscapeEditorObject.h"
#include "LandscapeEditorUtils.h"

#include "Editor/LandscapeEditor/Private/LandscapeImageFileCache.h"
#include "LandscapeEditorModule.h"
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

static int32 NumLandscapeRegions(ULandscapeInfo* InLandscapeInfo)
{
#if WITH_EDITOR
	int32 NumRegions = 0;
	TArray<AActor*> Children;
	InLandscapeInfo->LandscapeActor->GetAttachedActors(Children);
	TArray<ALocationVolume*> LandscapeRegions;

	for (AActor* Child : Children)
	{
		NumRegions += Child->IsA<ALocationVolume>() ? 1 : 0;
	}

	return NumRegions;
#endif
}

// Sets default values for this component's properties
UOCGLandscapeGenerateComponent::UOCGLandscapeGenerateComponent()
	: ColorRVTAsset(FSoftObjectPath(TEXT("/OneButtonLevelGeneration/RVT/RVT_Color.RVT_Color")))
	, HeightRVTAsset(FSoftObjectPath(TEXT("/OneButtonLevelGeneration/RVT/RVT_Height.RVT_Height")))
	, DisplacementRVTAsset(FSoftObjectPath(TEXT("/OneButtonLevelGeneration/RVT/RVT_Displacement.RVT_Displacement")))
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// // ...
 //    static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> ColorRVTFinder(TEXT("/Script/Engine.RuntimeVirtualTexture'/OneButtonLevelGeneration/RVT/RVT_Color.RVT_Color'"));
 //    if (ColorRVTFinder.Succeeded())
 //    {
 //        ColorRVT = ColorRVTFinder.Object;
 //    }
 //    else
 //    {
 //        UE_LOG(LogTemp, Warning, TEXT("Failed to find default Color RuntimeVirtualTexture at the specified path."));
 //    }
 //
 //    static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> HeightRVTFinder(TEXT("/Script/Engine.RuntimeVirtualTexture'/OneButtonLevelGeneration/RVT/RVT_Height.RVT_Height'"));
 //    if (HeightRVTFinder.Succeeded())
 //    {
 //        HeightRVT = HeightRVTFinder.Object;
 //    }
 //    else
 //    {
 //        UE_LOG(LogTemp, Warning, TEXT("Failed to find default Height RuntimeVirtualTexture at the specified path."));
 //    }
 //
 //    static ConstructorHelpers::FObjectFinder<URuntimeVirtualTexture> DisplacementRVTFinder(TEXT("/Script/Engine.RuntimeVirtualTexture'/OneButtonLevelGeneration/RVT/RVT_Displacement.RVT_Displacement'"));
 //    if (DisplacementRVTFinder.Succeeded())
 //    {
 //        DisplacementRVT = DisplacementRVTFinder.Object;
 //    }
 //    else
 //    {
 //        UE_LOG(LogTemp, Warning, TEXT("Failed to find default Displacement RuntimeVirtualTexture at the specified path."));
 //    }
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
    UMapPreset* MapPreset = nullptr;
	if (LevelGenerator)
	{
		MapPreset = LevelGenerator->GetMapPreset();
	}
	
    {	// 기존 UE5 FLandscapeEditorDetailCustomization_NewLandscape::OnCreateButtonClicked()의 Landscape 로직을 Copy한 것
    	//InitializeLandscapeSetting(World);
    }
	

    if (!World || World->IsGameWorld()) // 에디터에서만 실행되도록 확인
    {
        UE_LOG(LogTemp, Error, TEXT("유효한 에디터 월드가 아닙니다."));
        return;
    }

    {	// 기존 UE5 FLandscapeEditorDetailCustomization_NewLandscape::OnCreateButtonClicked()의 Landscape 로직을 Copy한 것
    	// const float TotalHeight = MapPreset->MaxHeight - MapPreset->MinHeight;
    	//
    	// const float RawScaleX = MapPreset->Landscape_Kilometer * 1000.f * 100.f / MapResolution.X;
    	// const float RawScaleY = MapPreset->Landscape_Kilometer * 1000.f * 100.f / MapResolution.Y;
    	// const float RawScaleZ = TotalHeight * 100.f * 0.001953125f;
    	//
    	// // 올림 후 uint32 변환
    	// const uint32 ScaleX = static_cast<uint32>(FMath::CeilToFloat(RawScaleX));
    	// const uint32 ScaleY = static_cast<uint32>(FMath::CeilToFloat(RawScaleY));
    	// const uint32 ScaleZ = static_cast<uint32>(FMath::CeilToFloat(RawScaleZ));
    	//    
    	// const FVector LandscapeScale = FVector(ScaleX, ScaleY , ScaleZ);
    	// const FVector Offset = FTransform(MapPreset->Landscape_Rotation, FVector::ZeroVector, LandscapeScale).TransformVector(FVector(-MapPreset->Landscape_ComponentCount.X * QuadsPerComponent / 2, -MapPreset->Landscape_ComponentCount.Y * QuadsPerComponent / 2, 0));
    	//
    	// TargetLandscape = World->SpawnActor<ALandscape>(MapPreset->Landscape_Location + Offset, MapPreset->Landscape_Rotation);
    	//TargetLandscape->SetActorRelativeScale3D(LandscapeScale);
    }

	bool IsCreateNewLandscape = false;
	if (ShouldCreateNewLandscape(MapPreset))
	{
		TArray<ALandscapeStreamingProxy*> ProxiesToDelete;
		for (TActorIterator<ALandscapeStreamingProxy> It(World); It; ++It)
		{
			ALandscapeStreamingProxy* Proxy = *It;
			if (Proxy && Proxy->GetLandscapeActor() == TargetLandscape)
			{ 
				ProxiesToDelete.Add(Proxy);
			}
		}
		
		// 2. Proxy 삭제
		for (ALandscapeStreamingProxy* Proxy : ProxiesToDelete)
		{
			if (Proxy)
			{
				Proxy->Destroy();
			}
		}
		
		// 3. Landscape 삭제
		if (TargetLandscape)
		{
			TargetLandscape->Destroy();
		}
		
		TargetLandscape = World->SpawnActor<ALandscape>();
		if (!TargetLandscape)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn ALandscape actor."));
			return;
		}
		IsCreateNewLandscape = true;
	}

    TargetLandscape->bCanHaveLayersContent = true;
    TargetLandscape->LandscapeMaterial = MapPreset->LandscapeMaterial;

	FIntPoint MapResolution = MapPreset->MapResolution;
	
	QuadsPerSection = static_cast<uint32>(MapPreset->Landscape_QuadsPerSection);
	ComponentCountX = (MapResolution.X - 1) / (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent);
	ComponentCountY = (MapResolution.Y - 1) / (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent);
	QuadsPerComponent = MapPreset->Landscape_SectionsPerComponent * QuadsPerSection;
    
	SizeX = ComponentCountX * QuadsPerComponent + 1;
	SizeY = ComponentCountY * QuadsPerComponent + 1;
	const int32 NumPixels = SizeX * SizeY;
	
	if ((MapResolution.X - 1) % (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent) != 0 || (MapResolution.Y - 1) % (QuadsPerSection * MapPreset->Landscape_SectionsPerComponent) != 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("LandscapeSize is not a recommended value."));
	}
	
	TargetLandscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((SizeX * SizeY) / (2048 * 2048) + 1), static_cast<uint32>(2));
	
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

    //랜드스케이프의 기본 속성 설정
    float OffsetX = (-MapPreset->MapResolution.X / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetY = (-MapPreset->MapResolution.Y / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetZ = (MapPreset->MaxHeight - MapPreset->MinHeight) / 2.f;
    TargetLandscape->SetActorLocation(FVector(OffsetX, OffsetY, 0));
    TargetLandscape->SetActorScale3D(FVector(100.0f * MapPreset->LandscapeScale, 100.0f * MapPreset->LandscapeScale, LandscapeZScale));

	if (IsCreateNewLandscape)
	{
		//Import 함수 호출 (변경된 시그니처에 맞춰서)
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

		{	// 기존 UE5 FLandscapeEditorDetailCustomization_NewLandscape::OnCreateButtonClicked()의 Landscape 로직을 Copy한 것
			// TargetLandscape->Import(
			// 	FGuid::NewGuid(),
			// 	0, 0,
			// 	SizeX - 1, SizeY - 1,
			// 	MapPreset->Landscape_SectionsPerComponent,
			// 	QuadsPerSection,
			// 	HeightmapDataPerLayer,
			// 	nullptr,
			// 	MaterialLayerDataPerLayer,
			// 	ELandscapeImportAlphamapType::Additive,
			// 	TArrayView<const FLandscapeLayer>() // 빈 TArray로부터 TArrayView 생성하여 전달
			// );    
		}
		ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
		
		FActorLabelUtilities::SetActorLabelUnique(TargetLandscape, ALandscape::StaticClass()->GetName());
		
		LandscapeInfo->UpdateLayerInfoMap(TargetLandscape);
	
		AddTargetLayers(TargetLandscape, MaterialLayerDataPerLayer);
	
		ManageLandscapeRegions(World, TargetLandscape);
	
		// 액터 등록 및 뷰포트 업데이트
		TargetLandscape->RegisterAllComponents();
		GEditor->RedrawAllViewports();
	
		//TargetLandscape->ForceUpdateLayersContent(true);

		// 1) 저장할 레벨 패키지 가져오기
		ULevel* Level = World->PersistentLevel;
		UPackage* LevelPackage = Level->GetOutermost();

		TargetLandscape->RuntimeVirtualTextures.Add(ColorRVT);
		TargetLandscape->RuntimeVirtualTextures.Add(HeightRVT);
		TargetLandscape->RuntimeVirtualTextures.Add(DisplacementRVT);
	}
	else
	{
		ImportMapDatas(World, *MaterialLayerDataPerLayer.Find(LayerGuid));

	}

	//TargetLandscape->MarkComponentsRenderStateDirty();
	// 또는
	TargetLandscape->ReregisterAllComponents();
	CreateRuntimeVirtualTextureVolume(TargetLandscape);
	//CachePointsForRiverGeneration();
#endif
}

void UOCGLandscapeGenerateComponent::ImportMapDatas(UWorld* World, TArray<FLandscapeImportLayerInfo> ImportLayers)
{
#if WITH_EDITOR
	if (World == nullptr)
		return;

	if (TargetLandscape == nullptr)
		return;
	
	if (ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo())
	{
		FIntRect LandscapeExtent;
		LandscapeInfo->GetLandscapeExtent(LandscapeExtent);

		LandscapeExtent.Max.X += 1;
		LandscapeExtent.Max.Y += 1;

		FIntRect ImportRegion = LandscapeExtent;

		FGuid CurrentLayerGuid = TargetLandscape->GetLayerConst(0)->Guid;

		const ELandscapeLayerPaintingRestriction PaintRestriction = ELandscapeLayerPaintingRestriction::None;
		const bool bIsWorldPartition = World->GetSubsystem<ULandscapeSubsystem>()->IsGridBased();
		if (bIsWorldPartition && NumLandscapeRegions(LandscapeInfo) > 0)
		{
			TArray<ALocationVolume*> LandscapeRegions;
			TArray<AActor*> Children;
			LandscapeInfo->LandscapeActor->GetAttachedActors(Children);
			for (AActor* Child : Children)
			{
				if (Child->IsA<ALocationVolume>())
				{
					LandscapeRegions.Add(Cast<ALocationVolume>(Child));
				}
			}

			int32 NumRegions = LandscapeRegions.Num();

			FScopedSlowTask Progress(static_cast<float>(NumRegions), NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "Importing Landscape Regions", "Importing Landscape Regions"));
			Progress.MakeDialog(/*bShowCancelButton = */ false);

			auto RegionImporter = [this, &ImportLayers, &Progress, LandscapeInfo, CurrentLayerGuid, PaintRestriction](const FBox& RegionBounds, const TArray<ALandscapeProxy*>& Proxies)
			{
				FIntRect LandscapeLoadedExtent;
				LandscapeInfo->GetLandscapeExtent(LandscapeLoadedExtent);
				LandscapeLoadedExtent.Max.X += 1;
				LandscapeLoadedExtent.Max.Y += 1;

				Progress.EnterProgressFrame(1.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "Importing Landscape Regions", "Importing Landscape Regions"));
				{
					ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
					FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
					FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
					HeightmapAccessor.SetData(LandscapeLoadedExtent.Min.X, LandscapeLoadedExtent.Min.Y, LandscapeLoadedExtent.Max.X - 1, LandscapeLoadedExtent.Max.Y - 1, GetLevelGenerator()->GetHeightMapData().GetData());
				}
				
				for (const FLandscapeImportLayerInfo& ImportLayer : ImportLayers)
				{
					ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
					FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
					FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, ImportLayer.LayerInfo);
					AlphamapAccessor.SetData(LandscapeLoadedExtent.Min.X, LandscapeLoadedExtent.Min.Y, LandscapeLoadedExtent.Max.X - 1, LandscapeLoadedExtent.Max.Y - 1, ImportLayer.LayerData.GetData(), PaintRestriction);
				}

				return !Progress.ShouldCancel();
			};
				
			ForEachRegion_LoadProcessUnload(LandscapeInfo, ImportRegion, World, RegionImporter);
		}
		else
		{
			FScopedSlowTask Progress(static_cast<float>(1 + ImportLayers.Num()), NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ImportingLandscape", "Importing Landscape"));
			Progress.MakeDialog(/*bShowCancelButton = */ false);

			{
				Progress.EnterProgressFrame(1.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ImportingLandscapeHeight", "Importing Landscape Height"));
				ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
				FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
				FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
				HeightmapAccessor.SetData(ImportRegion.Min.X, ImportRegion.Min.Y, ImportRegion.Max.X - 1, ImportRegion.Max.Y - 1, GetLevelGenerator()->GetHeightMapData().GetData());
			}

			for (const FLandscapeImportLayerInfo& ImportLayer : ImportLayers)
			{
				Progress.EnterProgressFrame(1.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ImportingLandscapeWeight", "Importing Landscape Weight"));
				
				ALandscape* Landscape = LandscapeInfo->LandscapeActor.Get();
				FScopedSetLandscapeEditingLayer Scope(Landscape, CurrentLayerGuid, [&] { check(Landscape); Landscape->RequestLayersContentUpdate(ELandscapeLayerUpdateMode::Update_Heightmap_All); });
				FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, ImportLayer.LayerInfo);
				AlphamapAccessor.SetData(ImportRegion.Min.X, ImportRegion.Min.Y, ImportRegion.Max.X - 1, ImportRegion.Max.Y - 1, ImportLayer.LayerData.GetData(), PaintRestriction);
			}
			
			LandscapeInfo->ForceLayersFullUpdate();
		}
	}
#endif
}

void UOCGLandscapeGenerateComponent::InitializeLandscapeSetting(const UWorld* World)
{
#if WITH_EDITOR
    AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
    
    bool bIsWorldPartition = false;
    if (World && World->GetSubsystem<ULandscapeSubsystem>())
    {
        bIsWorldPartition = World->GetSubsystem<ULandscapeSubsystem>()->IsGridBased();
    }
    
    bool bLandscapeLargerThanRegion = false;
    if (MapPreset)
    {
        bLandscapeLargerThanRegion = static_cast<int32>(MapPreset->WorldPartitionRegionSize) < MapPreset->Landscape_ComponentCount.X || static_cast<int32>(MapPreset->WorldPartitionRegionSize) < MapPreset->Landscape_ComponentCount.Y;
    }
    const bool bNeedsLandscapeRegions =  bIsWorldPartition && bLandscapeLargerThanRegion;

    // 랜드스케이프 생성
    QuadsPerSection = static_cast<uint32>(MapPreset->Landscape_QuadsPerSection);
    TotalLandscapeComponentSize = FIntPoint(MapPreset->Landscape_ComponentCount.X, MapPreset->Landscape_ComponentCount.Y);
    ComponentCountX = bNeedsLandscapeRegions ? FMath::Min(MapPreset->WorldPartitionRegionSize, TotalLandscapeComponentSize.X) : TotalLandscapeComponentSize.X;
    ComponentCountY = bNeedsLandscapeRegions ? FMath::Min(MapPreset->WorldPartitionRegionSize, TotalLandscapeComponentSize.Y) : TotalLandscapeComponentSize.Y;

    QuadsPerComponent = MapPreset->Landscape_SectionsPerComponent * QuadsPerSection;

    SizeX = ComponentCountX * QuadsPerComponent + 1;
    SizeY = ComponentCountY * QuadsPerComponent + 1;
#endif
}

void UOCGLandscapeGenerateComponent::AddTargetLayers(ALandscape* Landscape,
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

void UOCGLandscapeGenerateComponent::ManageLandscapeRegions(UWorld* World, ALandscape* Landscape)
{
#if WITH_EDITOR
    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    ALandscapeProxy* LandscapeProxy = nullptr;
	AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
	UMapPreset* MapPreset = nullptr;
	if (LevelGenerator)
	{
		MapPreset = LevelGenerator->GetMapPreset();
	}
     
    ULandscapeSubsystem* LandscapeSubsystem = World->GetSubsystem<ULandscapeSubsystem>();
	//LandscapeSubsystem->ChangeGridSize(LandscapeInfo, MapPreset->WorldPartitionGridSize);
    ChangeGridSize(World, LandscapeInfo, MapPreset->WorldPartitionGridSize);
	
    if (LandscapeInfo)
    {
        LandscapeProxy = LandscapeInfo->GetLandscapeProxy();
    }

	bool bIsWorldPartition = false;
	if (World && World->GetSubsystem<ULandscapeSubsystem>())
	{
		bIsWorldPartition = World->GetSubsystem<ULandscapeSubsystem>()->IsGridBased();
	}
    
	bool bLandscapeLargerThanRegion = false;
	
	if (MapPreset)
	{
		bLandscapeLargerThanRegion = static_cast<int32>(MapPreset->WorldPartitionRegionSize) < MapPreset->Landscape_ComponentCount.X || static_cast<int32>(MapPreset->WorldPartitionRegionSize) < MapPreset->Landscape_ComponentCount.Y;
	}
	const bool bNeedsLandscapeRegions = bIsWorldPartition && bLandscapeLargerThanRegion;

    if (bNeedsLandscapeRegions)
    {
    	TArray<FIntPoint> NewComponents;
    	NewComponents.Empty(TotalLandscapeComponentSize.X * TotalLandscapeComponentSize.Y);
    	for (int32 Y = 0; Y < TotalLandscapeComponentSize.Y; Y++)
    	{
    		for (int32 X = 0; X < TotalLandscapeComponentSize.X; X++)
    		{
    			NewComponents.Add(FIntPoint(X, Y));
    		}
    	}
    	
    	TArray<ALocationVolume*> RegionVolumes;
    	FBox LandscapeBounds;
    	auto AddComponentsToRegion = [this, World, LandscapeProxy, LandscapeInfo, LandscapeSubsystem, &RegionVolumes, &LandscapeBounds, MapPreset](const FIntPoint& RegionCoordinate, const TArray<FIntPoint>& NewComponents)
    	{
    		TRACE_CPUPROFILER_EVENT_SCOPE(AddComponentsToRegion);
    		
    		// Create a LocationVolume around the region 
    		int32 RegionSizeXTexels = QuadsPerSection * WorldPartitionRegionSize * MapPreset->Landscape_SectionsPerComponent;
    		int32 RegionSizeYTexels = QuadsPerSection * WorldPartitionRegionSize * MapPreset->Landscape_SectionsPerComponent;
		
    		double RegionSizeX = RegionSizeXTexels * LandscapeProxy->GetActorScale3D().X;
    		double RegionSizeY = RegionSizeYTexels * LandscapeProxy->GetActorScale3D().Y;
    		ALocationVolume* RegionVolume = CreateLandscapeRegionVolume(World, LandscapeProxy, RegionCoordinate, RegionSizeX);
    		if (RegionVolume)
    		{
    			RegionVolumes.Add(RegionVolume);
    		}
		
    		TArray<ALandscapeProxy*> CreatedStreamingProxies;
    		AddLandscapeComponent(LandscapeInfo, LandscapeSubsystem, NewComponents, CreatedStreamingProxies);
		
    		// ensures all the final height textures have been updated.
    		LandscapeInfo->ForceLayersFullUpdate();
    		TRACE_CPUPROFILER_EVENT_SCOPE(SaveCreatedActors);
    		UWorldPartition::FDisableNonDirtyActorTrackingScope Scope(World->GetWorldPartition(), true);
    		SaveObjects(MakeArrayView(CreatedStreamingProxies));
    		LandscapeBounds += LandscapeInfo->GetCompleteBounds();
			
    		return true;
    	};

    	SaveObjects(MakeArrayView(MakeArrayView(TArrayView<ALandscape*>(TArray<ALandscape*> { LandscapeInfo->LandscapeActor.Get() }))));

    	ForEachComponentByRegion(WorldPartitionRegionSize, NewComponents, AddComponentsToRegion);

    	// update the zcomponent of the volumes 
    	//const float ZScale =  LandscapeBounds.Max.Z - LandscapeBounds.Min.Z;
    	for (ALocationVolume* RegionVolume : RegionVolumes)
    	{
    		const FVector Scale = RegionVolume->GetActorScale();
    		const FVector NewScale{ Scale.X, Scale.Y, 1000000.0f}; //ZScale * 10.0 
    		RegionVolume->SetActorScale3D(NewScale);
    	}
    	
    	SaveObjects(MakeArrayView(RegionVolumes));
    	TArray<ALandscapeProxy*> AllProxies;
		
    	// save the initial region & unload it
    	LandscapeInfo->ForEachLandscapeProxy([&AllProxies](ALandscapeProxy* Proxy) {
			if (Proxy->IsA<ALandscapeStreamingProxy>())
			{		
				AllProxies.Add(Proxy);
				
			}
			return true;
		});

    	TRACE_CPUPROFILER_EVENT_SCOPE(SaveCreatedActors);
    	UWorldPartition::FDisableNonDirtyActorTrackingScope Scope(World->GetWorldPartition(), true);
    	SaveObjects(MakeArrayView(AllProxies));
    }
#endif
}

void UOCGLandscapeGenerateComponent::AddLandscapeComponent(ULandscapeInfo* InLandscapeInfo,
                                                           ULandscapeSubsystem* InLandscapeSubsystem, const TArray<FIntPoint>& InComponentCoordinates,
                                                           TArray<ALandscapeProxy*>& OutCreatedStreamingProxies)
{
    TArray<ULandscapeComponent*> NewComponents;
	InLandscapeInfo->Modify();

	for (const FIntPoint& ComponentCoordinate : InComponentCoordinates)
	{
		ULandscapeComponent* LandscapeComponent = InLandscapeInfo->XYtoComponentMap.FindRef(ComponentCoordinate);
		if (LandscapeComponent)
		{
			continue;
		}

		// Add New component...
		FIntPoint ComponentBase = ComponentCoordinate * InLandscapeInfo->ComponentSizeQuads;

		ALandscapeProxy* LandscapeProxy = InLandscapeSubsystem->FindOrAddLandscapeProxy(InLandscapeInfo, ComponentBase);
		if (!LandscapeProxy)
		{
			continue;
		}

		OutCreatedStreamingProxies.Add(LandscapeProxy);

		LandscapeComponent = NewObject<ULandscapeComponent>(LandscapeProxy, NAME_None, RF_Transactional);
		NewComponents.Add(LandscapeComponent);
		LandscapeComponent->Init(
			ComponentBase.X, ComponentBase.Y,
			LandscapeProxy->ComponentSizeQuads,
			LandscapeProxy->NumSubsections,
			LandscapeProxy->SubsectionSizeQuads
		);

		TArray<FColor> HeightData;
		const int32 ComponentVerts = (LandscapeComponent->SubsectionSizeQuads + 1) * LandscapeComponent->NumSubsections;
		const FColor PackedMidpoint = LandscapeDataAccess::PackHeight(LandscapeDataAccess::GetTexHeight(0.0f));
		HeightData.Init(PackedMidpoint, FMath::Square(ComponentVerts));

		LandscapeComponent->InitHeightmapData(HeightData, true);
		LandscapeComponent->UpdateMaterialInstances();

		InLandscapeInfo->XYtoComponentMap.Add(ComponentCoordinate, LandscapeComponent);
		InLandscapeInfo->XYtoAddCollisionMap.Remove(ComponentCoordinate);
	}

	// Need to register to use general height/xyoffset data update
	for (int32 Idx = 0; Idx < NewComponents.Num(); Idx++)
	{
		NewComponents[Idx]->RegisterComponent();
	}

	const bool bHasXYOffset = false;
	ALandscape* Landscape = InLandscapeInfo->LandscapeActor.Get();

	bool bHasLandscapeLayersContent = Landscape && Landscape->HasLayersContent();

	for (ULandscapeComponent* NewComponent : NewComponents)
	{
		if (bHasLandscapeLayersContent)
		{
			TArray<ULandscapeComponent*> ComponentsUsingHeightmap;
			ComponentsUsingHeightmap.Add(NewComponent);

			for (const FLandscapeLayer& Layer : Landscape->GetLayers())
			{
				// Since we do not share heightmap when adding new component, we will provided the required array, but they will only be used for 1 component
				TMap<UTexture2D*, UTexture2D*> CreatedHeightmapTextures;
				NewComponent->AddDefaultLayerData(Layer.Guid, ComponentsUsingHeightmap, CreatedHeightmapTextures);
			}
		}

		// Update Collision
		NewComponent->UpdateCachedBounds();
		NewComponent->UpdateBounds();
		NewComponent->MarkRenderStateDirty();

		if (!bHasLandscapeLayersContent)
		{
			ULandscapeHeightfieldCollisionComponent* CollisionComp = NewComponent->GetCollisionComponent();
			if (CollisionComp && !bHasXYOffset)
			{
				CollisionComp->MarkRenderStateDirty();
				CollisionComp->RecreateCollision();
			}
		}
	}

	if (Landscape)
	{
		GEngine->BroadcastOnActorMoved(Landscape);
	}
}

ALocationVolume* UOCGLandscapeGenerateComponent::CreateLandscapeRegionVolume(UWorld* InWorld,
    ALandscapeProxy* InParentLandscapeActor, const FIntPoint& InRegionCoordinate, double InRegionSize)
{
    const double Shrink = 0.95;

    FVector ParentLocation = InParentLandscapeActor->GetActorLocation();
    FVector Location = ParentLocation + FVector(InRegionCoordinate.X * InRegionSize, InRegionCoordinate.Y * InRegionSize, 0.0) + FVector(InRegionSize / 2.0, InRegionSize / 2.0, 0.0);
    FRotator Rotation;
    FActorSpawnParameters SpawnParameters;

    const FString Label = FString::Printf(TEXT("LandscapeRegion_%i_%i"), InRegionCoordinate.X, InRegionCoordinate.Y);
    SpawnParameters.Name = FName(*Label);
    SpawnParameters.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Requested;

    ALocationVolume* LocationVolume = InWorld->SpawnActor<ALocationVolume>(Location, Rotation, SpawnParameters);
    if (LocationVolume == nullptr)
    {
        return nullptr;
    }
    LocationVolume->SetActorLabel(Label);

    LocationVolume->AttachToActor(InParentLandscapeActor, FAttachmentTransformRules::KeepWorldTransform);
    const FVector Scale{ InRegionSize * Shrink,  InRegionSize * Shrink, InRegionSize * 0.5f };
    LocationVolume->SetActorScale3D(Scale);

    // Specify a cube shape for the LocationVolume
    UCubeBuilder* Builder = NewObject<UCubeBuilder>();
    Builder->X = 1.0f;
    Builder->Y = 1.0f;
    Builder->Z = 1.0f;
    UActorFactory::CreateBrushForVolumeActor(LocationVolume, Builder);

    LocationVolume->Tags.Add(FName("LandscapeRegion"));
    return LocationVolume;
}

void UOCGLandscapeGenerateComponent::ForEachComponentByRegion(const int32 RegionSize,
    const TArray<FIntPoint>& ComponentCoordinates,
    const TFunctionRef<bool(const FIntPoint&, const TArray<FIntPoint>&)>& RegionFn)
{
    if (RegionSize <= 0)
    {
        return;
    }

    TMap<FIntPoint, TArray< FIntPoint>> Regions;
    for (FIntPoint ComponentCoordinate : ComponentCoordinates)
    {
        FIntPoint RegionCoordinate;
        RegionCoordinate.X = (ComponentCoordinate.X) / RegionSize;
        RegionCoordinate.Y = (ComponentCoordinate.Y) / RegionSize;

        if (TArray<FIntPoint>* ComponentIndices = Regions.Find(RegionCoordinate))
        {
            ComponentIndices->Add(ComponentCoordinate);
        }
        else
        {
            Regions.Add(RegionCoordinate, TArray<FIntPoint> {ComponentCoordinate});
        }
    }

    auto Sorter = [](const FIntPoint& A, const FIntPoint& B)
    {
        if (A.Y == B.Y)
            return A.X < B.X;

        return A.Y < B.Y;
    };

    Regions.KeySort(Sorter);

    for (const TPair< FIntPoint, TArray< FIntPoint>>& RegionIt : Regions)
    {
        if (!RegionFn(RegionIt.Key, RegionIt.Value))
        {
            break;
        }
    }
}


void UOCGLandscapeGenerateComponent::ForEachRegion_LoadProcessUnload(ULandscapeInfo* InLandscapeInfo,
	const FIntRect& InDomain, const UWorld* InWorld,
	const TFunctionRef<bool(const FBox&, const TArray<ALandscapeProxy*>)>& InRegionFn)
{
#if WITH_EDITOR
	TArray<AActor*> Children;
	InLandscapeInfo->LandscapeActor->GetAttachedActors(Children);
	TArray<ALocationVolume*> LandscapeRegions;

	for (AActor* Child : Children)
	{
		if (Child->IsA<ALocationVolume>())
		{
			LandscapeRegions.Add(Cast<ALocationVolume>(Child));
		}
	}

	for (ALocationVolume* Region : LandscapeRegions)
	{
		Region->Load();

		FBox RegionBounds = Region->GetComponentsBoundingBox();

		TArray<AActor*> AllActors = InWorld->GetLevel(0)->Actors;
		TArray<ALandscapeProxy*> LandscapeProxies;
		for (AActor* Actor : AllActors)
		{
			if (ALandscapeStreamingProxy* Proxy = Cast<ALandscapeStreamingProxy>(Actor))
			{
				LandscapeProxies.Add(Proxy);
			}
		}

		// Save the actor
		const bool bShouldExit = !InRegionFn(RegionBounds, LandscapeProxies);

		InLandscapeInfo->ForceLayersFullUpdate();

		SaveObjects(MakeArrayView(LandscapeProxies));

		Region->Unload();

		if (bShouldExit)
		{
			break;
		}
	}
#endif
}

TMap<FGuid, TArray<FLandscapeImportLayerInfo>> UOCGLandscapeGenerateComponent::PrepareLandscapeLayerData(
    ALandscape* InTargetLandscape, AOCGLevelGenerator* InLevelGenerator, const UMapPreset* InMapPreset) const
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
	    if (ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo())
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
	for (ARuntimeVirtualTextureVolume* RVTVolume : CachedRuntimeVirtualTextureVolumes)
	{
		RVTVolume->Destroy();
	}
	CachedRuntimeVirtualTextureVolumes.Empty();
	
    TArray<URuntimeVirtualTexture*> VirtualTextureVolumesToCreate;
    GetMissingRuntimeVirtualTextureVolumes(InLandscapeActor, VirtualTextureVolumesToCreate);
    if (VirtualTextureVolumesToCreate.Num() == 0)
    {
        return false;
    }
    
    for (URuntimeVirtualTexture* VirtualTexture : VirtualTextureVolumesToCreate)
    {
    	ARuntimeVirtualTextureVolume* NewRVTVolume = InLandscapeActor->GetWorld()->SpawnActor<ARuntimeVirtualTextureVolume>();
    	NewRVTVolume->VirtualTextureComponent->SetVirtualTexture(VirtualTexture);
    	NewRVTVolume->VirtualTextureComponent->SetBoundsAlignActor(InLandscapeActor);
  
    	RuntimeVirtualTexture::SetBounds(NewRVTVolume->VirtualTextureComponent);
    	CachedRuntimeVirtualTextureVolumes.Add(NewRVTVolume);
    }

    return true;
#endif
}

bool UOCGLandscapeGenerateComponent::ChangeGridSize(const UWorld* InWorld, ULandscapeInfo* InLandscapeInfo,
	uint32 InNewGridSizeInComponents)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UOCGLandscapeGenerateComponent::ChangeGridSize);
	
	if (InWorld == nullptr)
		return false;

	ULandscapeSubsystem* LandscapeSubsystem = InWorld->GetSubsystem<ULandscapeSubsystem>();
	if (LandscapeSubsystem == nullptr || !LandscapeSubsystem->IsGridBased())
	{
		return false;
	}
		
	const uint32 GridSize = InLandscapeInfo->GetGridSize(InNewGridSizeInComponents);

	InLandscapeInfo->LandscapeActor->Modify();
	InLandscapeInfo->LandscapeActor->SetGridSize(GridSize);
	InLandscapeInfo->LandscapeActor->InitializeLandscapeLayersWeightmapUsage();
	
	InLandscapeInfo->LandscapeActor->bIncludeGridSizeInNameForLandscapeActors = true;

	FIntRect Extent;
	InLandscapeInfo->GetLandscapeExtent(Extent.Min.X, Extent.Min.Y, Extent.Max.X, Extent.Max.Y);

	const UWorld* World = InLandscapeInfo->LandscapeActor->GetWorld();
	UActorPartitionSubsystem* ActorPartitionSubsystem = World->GetSubsystem<UActorPartitionSubsystem>();

	TArray<ULandscapeComponent*> LandscapeComponents;
	LandscapeComponents.Reserve(InLandscapeInfo->XYtoComponentMap.Num());
	InLandscapeInfo->ForAllLandscapeComponents([&LandscapeComponents](ULandscapeComponent* LandscapeComponent)
	{
		LandscapeComponents.Add(LandscapeComponent);
	});
	
	{
		const uint32 ActorPartitionSubsystemGridSize = GridSize > 0 ? GridSize : ALandscapeStreamingProxy::StaticClass()->GetDefaultObject<APartitionActor>()->GetDefaultGridSize(World->PersistentLevel->GetWorld());
		const UActorPartitionSubsystem::FCellCoord MinCellCoords = UActorPartitionSubsystem::FCellCoord::GetCellCoord(Extent.Min, World->PersistentLevel, ActorPartitionSubsystemGridSize);
		const UActorPartitionSubsystem::FCellCoord MaxCellCoords = UActorPartitionSubsystem::FCellCoord::GetCellCoord(Extent.Max, World->PersistentLevel, ActorPartitionSubsystemGridSize);
	
		// 전체 스텝 수 계산
		const int32 NumX = MaxCellCoords.X - MinCellCoords.X + 1;
		const int32 NumY = MaxCellCoords.Y - MinCellCoords.Y + 1;
		const int32 TotalSteps = NumX * NumY;
		
		// FScopedSlowTask 생성
		FScopedSlowTask SlowTask(TotalSteps, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "Create LandscapeStreamingProxy", "Creating LandscapeStreamingProxies..."));
		SlowTask.MakeDialog(/*bShowCancelButton=*/ false);
    		
		FActorPartitionGridHelper::ForEachIntersectingCell(ALandscapeStreamingProxy::StaticClass(), Extent, World->PersistentLevel, [&SlowTask, TotalSteps, ActorPartitionSubsystem, InLandscapeInfo, InNewGridSizeInComponents, &LandscapeComponents](const UActorPartitionSubsystem::FCellCoord& CellCoord, const FIntRect& CellBounds)
		{
			// // 진행도 1 증가
			SlowTask.EnterProgressFrame(1.0f, FText::Format(
				NSLOCTEXT("Landscape", "ProcessingCell", "Processing Landscape cell {0}/{1}..."),
				FText::AsNumber(SlowTask.CompletedWork),
				FText::AsNumber(TotalSteps)
			));
			
			TMap<ULandscapeComponent*, UMaterialInterface*> ComponentMaterials;

			TArray<ULandscapeComponent*> ComponentsToMove;
			const int32 MaxComponents = static_cast<int32>(InNewGridSizeInComponents * InNewGridSizeInComponents);
			ComponentsToMove.Reserve(MaxComponents);
			for (int32 i = 0; i < LandscapeComponents.Num();)
			{
				if (ULandscapeComponent* LandscapeComponent = LandscapeComponents[i]; CellBounds.Contains(LandscapeComponent->GetSectionBase()))
				{
					ComponentMaterials.FindOrAdd(LandscapeComponent, LandscapeComponent->GetLandscapeMaterial());
					ComponentsToMove.Add(LandscapeComponent);
					LandscapeComponents.RemoveAtSwap(i);
				}
				else
				{
					i++;
				}
			}

			check(ComponentsToMove.Num() <= MaxComponents);
			if (ComponentsToMove.Num())
			{
				ALandscapeProxy* LandscapeProxy = UOCGLandscapeGenerateComponent::FindOrAddLandscapeStreamingProxy(ActorPartitionSubsystem, InLandscapeInfo, CellCoord);
				check(LandscapeProxy);
				InLandscapeInfo->MoveComponentsToProxy(ComponentsToMove, LandscapeProxy);
				
				for (ULandscapeComponent* MovedComponent : ComponentsToMove)
				{
					UMaterialInterface* PreviousLandscapeMaterial = ComponentMaterials.FindChecked(MovedComponent);
					
					MovedComponent->OverrideMaterial = nullptr;
					if (PreviousLandscapeMaterial != nullptr && PreviousLandscapeMaterial != MovedComponent->GetLandscapeMaterial())
					{
						if(LandscapeProxy->GetLandscapeMaterial() == LandscapeProxy->GetLandscapeActor()->GetLandscapeMaterial())
						{
							LandscapeProxy->LandscapeMaterial = PreviousLandscapeMaterial; 
						}
						else
						{
							MovedComponent->OverrideMaterial = PreviousLandscapeMaterial;
						}
					}
				}
			}

			return true;
		}, GridSize);
	}
	
	if (InLandscapeInfo->CanHaveLayersContent())
	{
		InLandscapeInfo->ForceLayersFullUpdate();
	}

	return true;
}

ALandscapeProxy* UOCGLandscapeGenerateComponent::FindOrAddLandscapeStreamingProxy(
	UActorPartitionSubsystem* InActorPartitionSubsystem, ULandscapeInfo* InLandscapeInfo,
	const UActorPartitionSubsystem::FCellCoord& InCellCoord)
{
	ALandscape* Landscape = InLandscapeInfo->LandscapeActor.Get();
	check(Landscape);

	auto LandscapeProxyCreated = [InCellCoord, Landscape](APartitionActor* PartitionActor)
	{
		const FIntPoint CellLocation(static_cast<int32>(InCellCoord.X) * Landscape->GetGridSize(), static_cast<int32>(InCellCoord.Y) * Landscape->GetGridSize());

		ALandscapeProxy* LandscapeProxy = CastChecked<ALandscapeProxy>(PartitionActor);
		// copy shared properties to this new proxy
		LandscapeProxy->SynchronizeSharedProperties(Landscape);
		const FVector ProxyLocation = Landscape->GetActorLocation() + FVector(CellLocation.X * Landscape->GetActorRelativeScale3D().X, CellLocation.Y * Landscape->GetActorRelativeScale3D().Y, 0.0f);

		LandscapeProxy->CreateLandscapeInfo();
		LandscapeProxy->SetActorLocationAndRotation(ProxyLocation, Landscape->GetActorRotation());
		LandscapeProxy->LandscapeSectionOffset = FIntPoint(CellLocation.X, CellLocation.Y);
		LandscapeProxy->SetIsSpatiallyLoaded(LandscapeProxy->GetLandscapeInfo()->AreNewLandscapeActorsSpatiallyLoaded());
	};

	const bool bCreate = true;
	const bool bBoundsSearch = false;

	ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(InActorPartitionSubsystem->GetActor(ALandscapeStreamingProxy::StaticClass(), InCellCoord, bCreate, InLandscapeInfo->LandscapeGuid, Landscape->GetGridSize(), bBoundsSearch, LandscapeProxyCreated));
	check(!LandscapeProxy || LandscapeProxy->GetGridSize() == Landscape->GetGridSize());
	return LandscapeProxy;
}

bool UOCGLandscapeGenerateComponent::ShouldCreateNewLandscape(UMapPreset* InMapPreset) const
{
	if (TargetLandscape == nullptr)
	{
		return true;
	}
	
	// 1) preset 로부터 기대값 계산
	int32 ExpectedQuadsPerSection    = int32(InMapPreset->Landscape_QuadsPerSection);
	int32 ExpectedSectionsPerComponent = InMapPreset->Landscape_SectionsPerComponent;
	int32 ExpectedQuadsPerComponent  = ExpectedQuadsPerSection * ExpectedSectionsPerComponent;
	int32 ExpectedComponentCountX    = (InMapPreset->MapResolution.X - 1) / ExpectedQuadsPerComponent;
	int32 ExpectedComponentCountY    = (InMapPreset->MapResolution.Y - 1) / ExpectedQuadsPerComponent;
	int32 ExpectedSizeX              = ExpectedComponentCountX * ExpectedQuadsPerComponent + 1;
	int32 ExpectedSizeY              = ExpectedComponentCountY * ExpectedQuadsPerComponent + 1;

	// 2) 비교
	if ( QuadsPerSection    != ExpectedQuadsPerSection
	  || QuadsPerComponent  != ExpectedQuadsPerComponent
	  || ComponentCountX    != ExpectedComponentCountX
	  || ComponentCountY    != ExpectedComponentCountY
	  || SizeX              != ExpectedSizeX
	  || SizeY              != ExpectedSizeY )
	{
		return true;
	}

	return false;
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


	if (TOptional<float> Height = TargetLandscape->GetHeightAtLocation(WorldLocation))
	{
		WorldLocation.Z = Height.GetValue();
	}
	else
	{
		WorldLocation.Z  = (HeightMapValue - 32768) / 128 * LandscapeZScale; // Adjust height based on the height map value and scale
	}
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
        (MaxHeight - MinHeight) * MapPreset->SeaLevel - 5;
    
    uint16 HighThreshold = SeaHeight + (MaxHeight - SeaHeight) * StartPointThresholdMultiplier;
    UE_LOG(LogTemp, Log, TEXT("High Threshold for River Start Point: %d"), HighThreshold);

    FVector LandscapeOrigin = TargetLandscape->GetLoadedBounds().GetCenter();
    FVector LandscapeExtent = TargetLandscape->GetLoadedBounds().GetExtent();
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

void UOCGLandscapeGenerateComponent::PostInitProperties()
{
	Super::PostInitProperties();
	
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		ColorRVT = ColorRVTAsset.LoadSynchronous();
		HeightRVT = HeightRVTAsset.LoadSynchronous();
		DisplacementRVT = DisplacementRVTAsset.LoadSynchronous();
		if (!ColorRVT)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load ColorRVT from %s"), *ColorRVTAsset.ToString());
		}
		if (!HeightRVT)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load HeightRVT from %s"), *HeightRVTAsset.ToString());
		}
		if (!DisplacementRVT)
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load DisplacementRVT from %s"), *DisplacementRVTAsset.ToString());
		}
	}
}
