// Copyright (c) 2025 Code1133. All rights reserved.

#include "Component/OCGLandscapeGenerateComponent.h"
#include "EngineUtils.h"

#include "ObjectTools.h"
#include "OCGLevelGenerator.h"
#include "OCGLog.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Data/MapPreset.h"

#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureVolume.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "RuntimeVirtualTextureSetBounds.h"

#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Utils/OCGLandscapeUtil.h"

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeSettings.h"
#if ENGINE_MINOR_VERSION > 5
#include "LandscapeEditLayer.h"
#endif
#include "LandscapeSubsystem.h"
#include "Utils/OCGFileUtils.h"
#include "Utils/OCGMaterialEditTool.h"

#include "LandscapeProxy.h"
#include "LocationVolume.h"
#include "Builders/CubeBuilder.h"
#include "LandscapeStreamingProxy.h"
#include "ActorFactories/ActorFactory.h"
#include "WorldPartition/WorldPartition.h"

#include "LandscapeConfigHelper.h"
#include "LandscapeEdit.h"

#endif

static void GetRuntimeVirtualTextureVolumes(ALandscape* InLandscapeActor, TArray<URuntimeVirtualTexture*>& OutVirtualTextures)
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

static TArray<ALocationVolume*> GetLandscapeRegionVolumes(const ALandscape* InLandscape)
{
#if WITH_EDITOR
	TArray<ALocationVolume*> LandscapeRegionVolumes;
	TArray<AActor*> Children;
	if (InLandscape)
	{
		InLandscape->GetAttachedActors(Children);
	}

	TArray<ALocationVolume*> LandscapeRegions;
	for (AActor* Child : Children)
	{
		if (ALocationVolume* LandscapeRegion = Cast<ALocationVolume>(Child))
		{
			LandscapeRegionVolumes.Add(LandscapeRegion);
		}
	}

	return LandscapeRegionVolumes;
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

ALandscape* UOCGLandscapeGenerateComponent::GetLandscape()
{
	if (TargetLandscape == nullptr)
	{
		if (TargetLandscapeAsset.ToSoftObjectPath().IsValid())
		{
			TargetLandscape = Cast<ALandscape>(TargetLandscapeAsset.Get());
			if (!IsValid(TargetLandscape))
			{
				TargetLandscape = Cast<ALandscape>(TargetLandscapeAsset.LoadSynchronous());
			}
		}
	}

	return TargetLandscape;
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
	
	if (MapPreset == nullptr)
		return;

    if (!World || World->IsGameWorld()) // 에디터에서만 실행되도록 확인
    {
        UE_LOG(LogOCGModule, Error, TEXT("유효한 에디터 월드가 아닙니다."));
        return;
    }

	bool IsCreateNewLandscape = false;
	if (ShouldCreateNewLandscape(World))
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
			for (ALocationVolume* Volume : GetLandscapeRegionVolumes(TargetLandscape))
			{
				Volume->Destroy();
			}
			
			TargetLandscape->Destroy();
		}

		Modify();
		if (AActor* Owner = GetOwner())
		{
			Owner->Modify();
			(void)Owner->MarkPackageDirty();
		}

		TargetLandscape = World->SpawnActor<ALandscape>();
		TargetLandscape->Modify();
		TargetLandscapeAsset = TargetLandscape;
		if (!TargetLandscape)
		{
			UE_LOG(LogOCGModule, Error, TEXT("Failed to spawn ALandscape actor."));
			return;
		}
		IsCreateNewLandscape = true;
	}
	else
	{
		ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
		FIntRect LandscapeExtent;
		LandscapeInfo->GetLandscapeExtent(LandscapeExtent);
	
		LandscapeExtent.Max.X += 1;
		LandscapeExtent.Max.Y += 1;
		int32 LandscapeResolution = LandscapeExtent.Max.X * LandscapeExtent.Max.Y;
		if (LandscapeResolution != MapPreset->MapResolution.X * MapPreset->MapResolution.Y)
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
				for (ALocationVolume* Volume : GetLandscapeRegionVolumes(TargetLandscape))
				{
					Volume->Destroy();
				}
			
				TargetLandscape->Destroy();
			}

			Modify();
			if (AActor* Owner = GetOwner())
			{
				Owner->Modify();
				(void)Owner->MarkPackageDirty();
			}
			
			TargetLandscape = World->SpawnActor<ALandscape>();
			TargetLandscape->Modify();
			TargetLandscapeAsset = TargetLandscape;
			if (!TargetLandscape)
			{
				UE_LOG(LogOCGModule, Error, TEXT("Failed to spawn ALandscape actor."));
				return;
			}
			IsCreateNewLandscape = true;
		}
	}
	
    TargetLandscape->bCanHaveLayersContent = true;
	if (TargetLandscape->LandscapeMaterial != MapPreset->LandscapeMaterial)
	{
		FScopedSlowTask SlowTask(5.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ChangingMaterial", "Change Landscape Material"));
		SlowTask.MakeDialog(); // 로딩 창 표시
		
		FProperty* MaterialProperty = FindFProperty<FProperty>(ALandscapeProxy::StaticClass(), "LandscapeMaterial");
		SlowTask.EnterProgressFrame(1.0f); // 진행도 갱신
		
		TargetLandscape->PreEditChange(MaterialProperty);
		SlowTask.EnterProgressFrame(1.0f); // 진행도 갱신
		
		TargetLandscape->LandscapeMaterial = MapPreset->LandscapeMaterial;
		SlowTask.EnterProgressFrame(1.0f); // 진행도 갱신
		
		FPropertyChangedEvent MaterialPropertyChangedEvent(MaterialProperty);
		SlowTask.EnterProgressFrame(1.0f); // 진행도 갱신
		
		TargetLandscape->PostEditChangeProperty(MaterialPropertyChangedEvent);
		SlowTask.EnterProgressFrame(); // 진행도 갱신
	}
	
	FIntPoint MapResolution = MapPreset->MapResolution;

	int32 PrevStaticLightingLOD = TargetLandscape->StaticLightingLOD;
	int32 CurStaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((LandscapeSetting.SizeX * LandscapeSetting.SizeY) / (2048 * 2048) + 1), static_cast<uint32>(2));
	if (PrevStaticLightingLOD != CurStaticLightingLOD)
	{
		FProperty* StaticLightingLODProperty = FindFProperty<FProperty>(ALandscapeProxy::StaticClass(), "StaticLightingLOD");
		TargetLandscape->StaticLightingLOD = CurStaticLightingLOD;
		FPropertyChangedEvent StaticLightingLODPropertyChangedEvent(StaticLightingLODProperty);
		TargetLandscape->PostEditChangeProperty(StaticLightingLODPropertyChangedEvent);
	}
	
    // Import 함수에 전달할 하이트맵 데이터를 TMap 형태로 포장
    // 키(Key)는 레이어의 고유 ID(GUID)이고, 값(Value)은 해당 레이어의 하이트맵 데이터입니다.
    // 우리는 기본 레이어 하나만 있으므로, 하나만 만들어줍니다.
    
    TMap<FGuid, TArray<uint16>> HeightmapDataPerLayer;
    FGuid LayerGuid = FGuid();
    HeightmapDataPerLayer.Add(LayerGuid, LevelGenerator->GetHeightMapData());

    // 레이어 데이터 준비
    const TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer = OCGLandscapeUtil::PrepareLandscapeLayerData(TargetLandscape, LevelGenerator, MapPreset);
	
    //랜드스케이프의 기본 속성 설정
    float OffsetX = (-MapPreset->MapResolution.X / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetY = (-MapPreset->MapResolution.Y / 2.f) * 100.f * MapPreset->LandscapeScale;
    TargetLandscape->SetActorLocation(FVector(OffsetX, OffsetY, LandscapeZOffset));
    TargetLandscape->SetActorScale3D(FVector(100.0f * MapPreset->LandscapeScale, 100.0f * MapPreset->LandscapeScale, LandscapeZScale));

	if (IsCreateNewLandscape)
	{
		// //Import 함수 호출 (변경된 시그니처에 맞춰서)
		TargetLandscape->Import(
			FGuid::NewGuid(),
			0, 0,
			MapResolution.X - 1, MapResolution.Y - 1,
			MapPreset->Landscape_SectionsPerComponent,
			LandscapeSetting.QuadsPerSection,
			HeightmapDataPerLayer,
			nullptr,
			MaterialLayerDataPerLayer,
			ELandscapeImportAlphamapType::Additive,
			TArrayView<const FLandscapeLayer>() // 빈 TArray로부터 TArrayView 생성하여 전달
		);

		ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
		
		FActorLabelUtilities::SetActorLabelUnique(TargetLandscape, ALandscape::StaticClass()->GetName());
		
		LandscapeInfo->UpdateLayerInfoMap(TargetLandscape);
	
		OCGLandscapeUtil::AddTargetLayers(TargetLandscape, MaterialLayerDataPerLayer);
	
		OCGLandscapeUtil::ManageLandscapeRegions(World, TargetLandscape, MapPreset, LandscapeSetting);
	
		// 액터 등록 및 뷰포트 업데이트
		TargetLandscape->RegisterAllComponents();
		GEditor->RedrawAllViewports();
	
		//TargetLandscape->ForceUpdateLayersContent(true);
		
		FProperty* RuntimeVirtualTexturesProperty = FindFProperty<FProperty>(ALandscapeProxy::StaticClass(), "RuntimeVirtualTextures");

		TargetLandscape->RuntimeVirtualTextures.Add(ColorRVT);
		TargetLandscape->RuntimeVirtualTextures.Add(HeightRVT);
		TargetLandscape->RuntimeVirtualTextures.Add(DisplacementRVT);
		
		FPropertyChangedEvent RuntimeVirtualTexturesPropertyChangedEvent(RuntimeVirtualTexturesProperty);
		TargetLandscape->PostEditChangeProperty(RuntimeVirtualTexturesPropertyChangedEvent);
	}
	else
	{
		OCGLandscapeUtil::ClearTargetLayers(TargetLandscape);
		OCGLandscapeUtil::AddTargetLayers(TargetLandscape, MaterialLayerDataPerLayer);
		OCGLandscapeUtil::ImportMapDatas(World, TargetLandscape, LevelGenerator->GetHeightMapData(), *MaterialLayerDataPerLayer.Find(LayerGuid));
	}
	
	TargetLandscape->ReregisterAllComponents();
	CreateRuntimeVirtualTextureVolume(TargetLandscape);
#endif
}

void UOCGLandscapeGenerateComponent::InitializeLandscapeSetting(const UWorld* World)
{
#if WITH_EDITOR
    AOCGLevelGenerator* LevelGenerator = GetLevelGenerator();
    const UMapPreset* MapPreset = LevelGenerator->GetMapPreset();
	
	LandscapeSetting.WorldPartitionGridSize = MapPreset->WorldPartitionGridSize;
	LandscapeSetting.WorldPartitionRegionSize = MapPreset->WorldPartitionRegionSize;

	LandscapeSetting.QuadsPerSection = static_cast<uint32>(MapPreset->Landscape_QuadsPerSection);
	LandscapeSetting.ComponentCountX = (MapPreset->MapResolution.X - 1) / (LandscapeSetting.QuadsPerSection * MapPreset->Landscape_SectionsPerComponent);
	LandscapeSetting.ComponentCountY = (MapPreset->MapResolution.Y - 1) / (LandscapeSetting.QuadsPerSection * MapPreset->Landscape_SectionsPerComponent);
	LandscapeSetting.QuadsPerComponent = MapPreset->Landscape_SectionsPerComponent * LandscapeSetting.QuadsPerSection;
	   
	LandscapeSetting.SizeX = LandscapeSetting.ComponentCountX * LandscapeSetting.QuadsPerComponent + 1;
	LandscapeSetting.SizeY = LandscapeSetting.ComponentCountY * LandscapeSetting.QuadsPerComponent + 1;
	
	if ((MapPreset->MapResolution.X - 1) % (LandscapeSetting.QuadsPerSection * MapPreset->Landscape_SectionsPerComponent) != 0 || (MapPreset->MapResolution.Y - 1) % (LandscapeSetting.QuadsPerSection * MapPreset->Landscape_SectionsPerComponent) != 0)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("LandscapeSize is not a recommended value."));
	}
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
	for (auto RVTVolumeAsset : CachedRuntimeVirtualTextureVolumeAssets)
	{
		CachedRuntimeVirtualTextureVolumes.Add(RVTVolumeAsset.LoadSynchronous());
	}
	
	for (ARuntimeVirtualTextureVolume* RVTVolume : CachedRuntimeVirtualTextureVolumes)
	{
		if (RVTVolume)
		{
			RVTVolume->Destroy();
		}
	}
	CachedRuntimeVirtualTextureVolumes.Empty();
	
    TArray<URuntimeVirtualTexture*> VirtualTextureVolumesToCreate;
    GetRuntimeVirtualTextureVolumes(InLandscapeActor, VirtualTextureVolumesToCreate);
    if (VirtualTextureVolumesToCreate.Num() == 0)
    {
        return false;
    }
    
    for (URuntimeVirtualTexture* VirtualTexture : VirtualTextureVolumesToCreate)
    {
    	Modify();
    	if (AActor* Owner = GetOwner())
    	{
    		Owner->Modify();
    		(void)Owner->MarkPackageDirty();
    	}
    	
    	ARuntimeVirtualTextureVolume* NewRVTVolume = InLandscapeActor->GetWorld()->SpawnActor<ARuntimeVirtualTextureVolume>();
    	NewRVTVolume->Modify();
    	CachedRuntimeVirtualTextureVolumeAssets.Add(NewRVTVolume);
    	
    	NewRVTVolume->VirtualTextureComponent->SetVirtualTexture(VirtualTexture);
    	NewRVTVolume->VirtualTextureComponent->SetBoundsAlignActor(InLandscapeActor);
    	NewRVTVolume->SetIsSpatiallyLoaded(false);
  
    	RuntimeVirtualTexture::SetBounds(NewRVTVolume->VirtualTextureComponent);
    	CachedRuntimeVirtualTextureVolumes.Add(NewRVTVolume);
    }

	if (!CachedRuntimeVirtualTextureVolumes.IsEmpty())
	{
		if (UBoxComponent* VolumeBox = CachedRuntimeVirtualTextureVolumes[0]->Box)
		{
			// 월드 스케일이 반영된 반경(half-extent)을 가져와서
			VolumeExtent = VolumeBox->GetScaledBoxExtent();
			VolumeOrigin = VolumeBox->GetComponentLocation();
		}
	}
	
    return true;
#endif
}

bool UOCGLandscapeGenerateComponent::ShouldCreateNewLandscape(const UWorld* World)
{
	const FLandscapeSetting PrevSetting = LandscapeSetting; // 이전 세팅 저장값
	InitializeLandscapeSetting(World);

	if (IsLandscapeSettingChanged(PrevSetting, LandscapeSetting))
	{
		return true;
	}
		
	if (!TargetLandscape)
	{
		if (TargetLandscapeAsset.ToSoftObjectPath().IsValid())
		{
			TargetLandscape = Cast<ALandscape>(TargetLandscapeAsset.Get());
			if (!IsValid(TargetLandscape))
			{
				TargetLandscape = Cast<ALandscape>(TargetLandscapeAsset.LoadSynchronous());
				if (IsValid(TargetLandscape))
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	
	return false;
}

bool UOCGLandscapeGenerateComponent::IsLandscapeSettingChanged(const FLandscapeSetting& Prev,
	const FLandscapeSetting& Curr)
{
	return Prev.WorldPartitionGridSize != Curr.WorldPartitionGridSize ||
		   Prev.WorldPartitionRegionSize != Curr.WorldPartitionRegionSize ||
		   Prev.QuadsPerSection != Curr.QuadsPerSection ||
		   Prev.TotalLandscapeComponentSize != Curr.TotalLandscapeComponentSize ||
		   Prev.ComponentCountX != Curr.ComponentCountX ||
		   Prev.ComponentCountY != Curr.ComponentCountY ||
		   Prev.QuadsPerComponent != Curr.QuadsPerComponent ||
	       Prev.SizeX != Curr.SizeX ||
	       Prev.SizeY != Curr.SizeY;
}

FVector UOCGLandscapeGenerateComponent::GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const
{
    if (!TargetLandscape || !GetLevelGenerator())
    {
        UE_LOG(LogOCGModule , Error, TEXT("TargetLandscape or MapPreset is not set. Cannot get world position."));
        return FVector::ZeroVector;
    }

    const UMapPreset* MapPreset = GetLevelGenerator()->GetMapPreset();

    float OffsetX = (-MapPreset->MapResolution.X / 2.f) * 100.f * MapPreset->LandscapeScale;
    float OffsetY = (-MapPreset->MapResolution.Y / 2.f) * 100.f * MapPreset->LandscapeScale;
	
    FVector WorldLocation = LandscapeOrigin + FVector(
        2 * (MapPoint.X / static_cast<float>(MapPreset->MapResolution.X)) * LandscapeExtent.X + OffsetX,
        2 * (MapPoint.Y / static_cast<float>(MapPreset->MapResolution.Y)) * LandscapeExtent.Y + OffsetY,
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
			UE_LOG(LogOCGModule, Warning, TEXT("Failed to load ColorRVT from %s"), *ColorRVTAsset.ToString());
		}
		if (!HeightRVT)
		{
			UE_LOG(LogOCGModule, Warning, TEXT("Failed to load HeightRVT from %s"), *HeightRVTAsset.ToString());
		}
		if (!DisplacementRVT)
		{
			UE_LOG(LogOCGModule, Warning, TEXT("Failed to load DisplacementRVT from %s"), *DisplacementRVTAsset.ToString());
		}
	}
}

void UOCGLandscapeGenerateComponent::OnRegister()
{
	Super::OnRegister();
#if WITH_EDITOR
	if (GetWorld() && GetWorld()->IsEditorWorld() && !TargetLandscapeAsset.IsValid())
	{
		// 명시적 확인
		if (TargetLandscapeAsset.ToSoftObjectPath().IsValid())
		{
			TargetLandscape = Cast<ALandscape>(TargetLandscapeAsset.Get());
			if (!TargetLandscape)
			{
				TargetLandscape = Cast<ALandscape>(TargetLandscapeAsset.LoadSynchronous());
			}
		}
	}
#endif
}
