// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGLandscapeGenSubsystem.h"

#include "OCGLog.h"
#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "Utils/OCGLandscapeUtil.h"

#include "Editor.h"
#include "EngineUtils.h"

#include "Landscape.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	#include "LandscapeEditLayer.h"
#endif
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "LandscapeStreamingProxy.h"

#include "Components/BoxComponent.h"
#include "Components/RuntimeVirtualTextureComponent.h"
#include "LocationVolume.h"
#include "RuntimeVirtualTextureSetBounds.h"
#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureVolume.h"

static void GetRuntimeVirtualTextureVolumes(ALandscape* InLandscapeActor, TArray<URuntimeVirtualTexture*>& OutVirtualTextures)
{
	UWorld* World = InLandscapeActor ? InLandscapeActor->GetWorld() : nullptr;
	if (!World)
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
		if (VirtualTexture && FoundVolumes.Find(VirtualTexture) == INDEX_NONE)
		{
			OutVirtualTextures.Add(VirtualTexture);
		}
	}
}

static TArray<ALocationVolume*> GetLandscapeRegionVolumes(const ALandscape* InLandscape)
{
	TArray<ALocationVolume*> LandscapeRegionVolumes;
	if (!InLandscape)
	{
	    return LandscapeRegionVolumes;
    }

	TArray<AActor*> Children;
	InLandscape->GetAttachedActors(Children);
	for (AActor* Child : Children)
	{
		if (ALocationVolume* Region = Cast<ALocationVolume>(Child))
		{
			LandscapeRegionVolumes.Add(Region);
		}
	}
	return LandscapeRegionVolumes;
}

void UOCGLandscapeGenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ColorRVT = Cast<URuntimeVirtualTexture>(FSoftObjectPath(TEXT("/OneButtonLevelGeneration/RVT/RVT_Color.RVT_Color")).TryLoad());
	HeightRVT = Cast<URuntimeVirtualTexture>(FSoftObjectPath(TEXT("/OneButtonLevelGeneration/RVT/RVT_Height.RVT_Height")).TryLoad());
	DisplacementRVT = Cast<URuntimeVirtualTexture>(FSoftObjectPath(TEXT("/OneButtonLevelGeneration/RVT/RVT_Displacement.RVT_Displacement")).TryLoad());

	if (!ColorRVT)        { UE_LOG(LogOCGModule, Warning, TEXT("Failed to load ColorRVT"));        }
	if (!HeightRVT)       { UE_LOG(LogOCGModule, Warning, TEXT("Failed to load HeightRVT"));       }
	if (!DisplacementRVT) { UE_LOG(LogOCGModule, Warning, TEXT("Failed to load DisplacementRVT")); }
}

void UOCGLandscapeGenSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UOCGLandscapeGenSubsystem::ApplyLandscape(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
	if (!IsValid(Preset))
	{
        return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World || World->IsGameWorld())
	{
		UE_LOG(LogOCGModule, Error, TEXT("Valid editor world not found."));
		return;
	}

	const float LandscapeZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;
	const float AbsMaxHeight    = FMath::Abs(Preset->MaxHeight);
	const float AbsMinHeight    = FMath::Abs(Preset->MinHeight);
	const float AbsOffset       = FMath::Abs(AbsMaxHeight - AbsMinHeight) / 2.0f;
	const float ZOffset         = (AbsMaxHeight < AbsMinHeight) ? -AbsOffset : AbsOffset;

	ModifyLandscapeWithBiome(Preset, DataContainer, LandscapeZScale, ZOffset);

	const bool bIsNewLandscape = ShouldCreateNewLandscape(Preset);
	if (bIsNewLandscape)
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
		for (ALandscapeStreamingProxy* Proxy : ProxiesToDelete)
		{
			if (Proxy) Proxy->Destroy();
		}
		if (TargetLandscape)
		{
			for (ALocationVolume* Volume : GetLandscapeRegionVolumes(TargetLandscape))
			{
				Volume->Destroy();
			}
			TargetLandscape->Destroy();
		}

		TargetLandscape = World->SpawnActor<ALandscape>();
		TargetLandscapeAsset = TargetLandscape;
		if (!TargetLandscape)
		{
			UE_LOG(LogOCGModule, Error, TEXT("Failed to spawn ALandscape."));
			return;
		}
		TargetLandscape->Modify();
	}

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 6
	TargetLandscape->bCanHaveLayersContent = true;
#endif

	if (TargetLandscape->LandscapeMaterial != Preset->LandscapeMaterial)
	{
		FScopedSlowTask SlowTask(5.0f, NSLOCTEXT("ONEBUTTONLEVELGENERATION_API", "ChangingMaterial", "Change Landscape Material"));
		SlowTask.MakeDialog();
		FProperty* MaterialProperty = FindFProperty<FProperty>(ALandscapeProxy::StaticClass(), "LandscapeMaterial");
		SlowTask.EnterProgressFrame(1.0f);
		TargetLandscape->PreEditChange(MaterialProperty);
		SlowTask.EnterProgressFrame(1.0f);
		TargetLandscape->LandscapeMaterial = Preset->LandscapeMaterial;
		SlowTask.EnterProgressFrame(1.0f);
		FPropertyChangedEvent MaterialPropertyChangedEvent(MaterialProperty);
		SlowTask.EnterProgressFrame(1.0f);
		TargetLandscape->PostEditChangeProperty(MaterialPropertyChangedEvent);
		SlowTask.EnterProgressFrame();
	}

	const int32 CurStaticLightingLOD = FMath::DivideAndRoundUp(
		FMath::CeilLogTwo((LandscapeSetting.SizeX * LandscapeSetting.SizeY) / (2048 * 2048) + 1),
		static_cast<uint32>(2)
    );
	if (TargetLandscape->StaticLightingLOD != CurStaticLightingLOD)
	{
		FProperty* StaticLightingLODProperty = FindFProperty<FProperty>(ALandscapeProxy::StaticClass(), "StaticLightingLOD");
		TargetLandscape->StaticLightingLOD = CurStaticLightingLOD;
		FPropertyChangedEvent StaticLightingLODPropertyChangedEvent(StaticLightingLODProperty);
		TargetLandscape->PostEditChangeProperty(StaticLightingLODPropertyChangedEvent);
	}

	const FIntPoint MapResolution = Preset->MapResolution;
	const float OffsetX = (-MapResolution.X / 2.f) * 100.f * Preset->LandscapeScale;
	const float OffsetY = (-MapResolution.Y / 2.f) * 100.f * Preset->LandscapeScale;
	TargetLandscape->SetActorLocation(FVector(OffsetX, OffsetY, ZOffset));
	TargetLandscape->SetActorScale3D(FVector(100.f * Preset->LandscapeScale, 100.f * Preset->LandscapeScale, LandscapeZScale));

	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> MaterialLayerDataPerLayer =
		OCGLandscapeUtil::PrepareLandscapeLayerData(TargetLandscape, DataContainer.WeightLayers, Preset);
	const FGuid LayerGuid = FGuid();

	if (bIsNewLandscape)
	{
		TMap<FGuid, TArray<uint16>> HeightmapDataPerLayer;
		HeightmapDataPerLayer.Add(LayerGuid, DataContainer.HeightMapData);

		TargetLandscape->Import(
			FGuid::NewGuid(),
			0, 0,
			MapResolution.X - 1, MapResolution.Y - 1,
			Preset->Landscape_SectionsPerComponent,
			LandscapeSetting.QuadsPerSection,
			HeightmapDataPerLayer,
			nullptr,
			MaterialLayerDataPerLayer,
			ELandscapeImportAlphamapType::Additive,
			TArrayView<const FLandscapeLayer>()
		);

		ULandscapeInfo* LandscapeInfo = TargetLandscape->GetLandscapeInfo();
		TargetLandscape->SetActorLabel(ALandscape::StaticClass()->GetName());
		LandscapeInfo->UpdateLayerInfoMap(TargetLandscape);
		OCGLandscapeUtil::AddTargetLayers(TargetLandscape, MaterialLayerDataPerLayer);
		// ManageLandscapeRegions takes non-const UMapPreset* for legacy reasons; does not modify it
		OCGLandscapeUtil::ManageLandscapeRegions(World, TargetLandscape, const_cast<UMapPreset*>(Preset), LandscapeSetting);

		FProperty* RVTProperty = FindFProperty<FProperty>(ALandscapeProxy::StaticClass(), "RuntimeVirtualTextures");
		TargetLandscape->RuntimeVirtualTextures.Add(ColorRVT);
		TargetLandscape->RuntimeVirtualTextures.Add(HeightRVT);
		TargetLandscape->RuntimeVirtualTextures.Add(DisplacementRVT);
		FPropertyChangedEvent RVTPropertyChangedEvent(RVTProperty);
		TargetLandscape->PostEditChangeProperty(RVTPropertyChangedEvent);
	}
	else
	{
		OCGLandscapeUtil::ClearTargetLayers(TargetLandscape);
		OCGLandscapeUtil::AddTargetLayers(TargetLandscape, MaterialLayerDataPerLayer);
		OCGLandscapeUtil::ImportMapDatas(World, TargetLandscape, DataContainer.HeightMapData, *MaterialLayerDataPerLayer.Find(LayerGuid));
	}

	TargetLandscape->ReregisterAllComponents();
	CreateRuntimeVirtualTextureVolume(TargetLandscape);
}

FVector UOCGLandscapeGenSubsystem::GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const UMapPreset* Preset, const TArray<uint16>* InHeightMapData) const
{
	if (!TargetLandscape || !Preset)
	{
		UE_LOG(LogOCGModule, Error, TEXT("TargetLandscape or Preset is not set."));
		return FVector::ZeroVector;
	}

	const float OffsetX = (-Preset->MapResolution.X / 2.f) * 100.f * Preset->LandscapeScale;
	const float OffsetY = (-Preset->MapResolution.Y / 2.f) * 100.f * Preset->LandscapeScale;

	FVector WorldLocation = VolumeOrigin + FVector(
		2.f * (MapPoint.X / static_cast<float>(Preset->MapResolution.X)) * VolumeExtent.X + OffsetX,
		2.f * (MapPoint.Y / static_cast<float>(Preset->MapResolution.Y)) * VolumeExtent.Y + OffsetY,
		0.0f
	);

	if (TOptional<float> Height = TargetLandscape->GetHeightAtLocation(WorldLocation))
	{
		WorldLocation.Z = Height.GetValue();
	}
	else if (InHeightMapData)
	{
		const int32 Index = MapPoint.Y * Preset->MapResolution.X + MapPoint.X;
		if (InHeightMapData->IsValidIndex(Index))
		{
			const float LandscapeZScale = (Preset->MaxHeight - Preset->MinHeight) * 0.001953125f;
			WorldLocation.Z = ((*InHeightMapData)[Index] - 32768.f) / 128.f * LandscapeZScale;
		}
	}

	return WorldLocation;
}

void UOCGLandscapeGenSubsystem::InitializeLandscapeSetting(const UMapPreset* Preset)
{
	LandscapeSetting.WorldPartitionGridSize   = Preset->WorldPartitionGridSize;
	LandscapeSetting.WorldPartitionRegionSize = Preset->WorldPartitionRegionSize;
	LandscapeSetting.QuadsPerSection          = static_cast<uint32>(Preset->Landscape_QuadsPerSection);
	LandscapeSetting.ComponentCountX          = (Preset->MapResolution.X - 1) / (LandscapeSetting.QuadsPerSection * Preset->Landscape_SectionsPerComponent);
	LandscapeSetting.ComponentCountY          = (Preset->MapResolution.Y - 1) / (LandscapeSetting.QuadsPerSection * Preset->Landscape_SectionsPerComponent);
	LandscapeSetting.QuadsPerComponent        = Preset->Landscape_SectionsPerComponent * LandscapeSetting.QuadsPerSection;
	LandscapeSetting.SizeX                    = LandscapeSetting.ComponentCountX * LandscapeSetting.QuadsPerComponent + 1;
	LandscapeSetting.SizeY                    = LandscapeSetting.ComponentCountY * LandscapeSetting.QuadsPerComponent + 1;

	if ((Preset->MapResolution.X - 1) % (LandscapeSetting.QuadsPerSection * Preset->Landscape_SectionsPerComponent) != 0 ||
		(Preset->MapResolution.Y - 1) % (LandscapeSetting.QuadsPerSection * Preset->Landscape_SectionsPerComponent) != 0)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("LandscapeSize is not a recommended value."));
	}
}

bool UOCGLandscapeGenSubsystem::ShouldCreateNewLandscape(const UMapPreset* Preset)
{
	const FLandscapeSetting PrevSetting = LandscapeSetting;
	InitializeLandscapeSetting(Preset);

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
				if (IsValid(TargetLandscape)) { return false; }
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

bool UOCGLandscapeGenSubsystem::IsLandscapeSettingChanged(const FLandscapeSetting& Prev, const FLandscapeSetting& Curr)
{
	return Prev.WorldPartitionGridSize   != Curr.WorldPartitionGridSize
		|| Prev.WorldPartitionRegionSize != Curr.WorldPartitionRegionSize
		|| Prev.QuadsPerSection          != Curr.QuadsPerSection
		|| Prev.ComponentCountX          != Curr.ComponentCountX
		|| Prev.ComponentCountY          != Curr.ComponentCountY
		|| Prev.QuadsPerComponent        != Curr.QuadsPerComponent
		|| Prev.SizeX                    != Curr.SizeX
		|| Prev.SizeY                    != Curr.SizeY;
}

bool UOCGLandscapeGenSubsystem::CreateRuntimeVirtualTextureVolume(ALandscape* InLandscape)
{
	if (!InLandscape)
	{
        return false;
    }

	for (auto& RVTVolumeAsset : CachedRuntimeVirtualTextureVolumeAssets)
	{
		CachedRuntimeVirtualTextureVolumes.Add(RVTVolumeAsset.LoadSynchronous());
	}
	for (ARuntimeVirtualTextureVolume* RVTVolume : CachedRuntimeVirtualTextureVolumes)
	{
		if (RVTVolume) RVTVolume->Destroy();
	}
	CachedRuntimeVirtualTextureVolumes.Empty();

	TArray<URuntimeVirtualTexture*> VirtualTextureVolumesToCreate;
	GetRuntimeVirtualTextureVolumes(InLandscape, VirtualTextureVolumesToCreate);
	if (VirtualTextureVolumesToCreate.IsEmpty())
	{
        return false;
    }

	for (URuntimeVirtualTexture* VirtualTexture : VirtualTextureVolumesToCreate)
	{
		ARuntimeVirtualTextureVolume* NewRVTVolume = InLandscape->GetWorld()->SpawnActor<ARuntimeVirtualTextureVolume>();
		NewRVTVolume->Modify();
		CachedRuntimeVirtualTextureVolumeAssets.Add(NewRVTVolume);

		NewRVTVolume->VirtualTextureComponent->SetVirtualTexture(VirtualTexture);
		NewRVTVolume->VirtualTextureComponent->SetBoundsAlignActor(InLandscape);
		NewRVTVolume->SetIsSpatiallyLoaded(false);

		RuntimeVirtualTexture::SetBounds(NewRVTVolume->VirtualTextureComponent);
		CachedRuntimeVirtualTextureVolumes.Add(NewRVTVolume);
	}

	if (!CachedRuntimeVirtualTextureVolumes.IsEmpty())
	{
		if (const UBoxComponent* VolumeBox = CachedRuntimeVirtualTextureVolumes[0]->Box)
		{
			VolumeExtent = VolumeBox->GetScaledBoxExtent();
			VolumeOrigin = VolumeBox->GetComponentLocation();
		}
	}

	return true;
}

void UOCGLandscapeGenSubsystem::ModifyLandscapeWithBiome(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer, float ZScale, float ZOffset)
{
	if (!Preset->bModifyTerrainByBiome)
	{
        return;
	}

	TArray<float> MinHeights;
	TArray<float> BlurredMinHeights;

	const float HeightRange = Preset->MaxHeight - Preset->MinHeight;
	CalculateBiomeMinHeights(Preset, DataContainer.HeightMapData, DataContainer.BiomeMap, MinHeights, ZScale, ZOffset);

	if (Preset->BiomeHeightBlendRadius > 0)
	{
		BlurBiomeMinHeights(Preset, MinHeights, BlurredMinHeights);
    }
	else
	{
		BlurredMinHeights = MinHeights;
    }

	const float SeaLevelHeightF = Preset->bContainWater ? Preset->SeaLevel * HeightRange + Preset->MinHeight : 0.f;
	const uint16 SeaLevelHeight = static_cast<uint16>((SeaLevelHeightF - ZOffset) * 128.f / ZScale + 32768.f);

	for (int32 Y = 0; Y < Preset->MapResolution.Y; ++Y)
	{
		for (int32 X = 0; X < Preset->MapResolution.X; ++X)
		{
			const int32 Index = Y * Preset->MapResolution.X + X;
			const FOCGBiomeSettings* CurrentBiome = DataContainer.BiomeMap[Index];
			if (!CurrentBiome || CurrentBiome->BiomeName == TEXT("Water")) continue;

			const uint16 CurrentHeight = DataContainer.HeightMapData[Index];
			float MtoPRatio = 0.f;

			for (int32 I = 1; I < DataContainer.WeightLayers.Num(); ++I)
			{
				const FName LayerName(FString::Printf(TEXT("Layer%d"), I));
				const float CurrentBiomeWeight = DataContainer.WeightLayers[LayerName][Index] / 255.f;
				if (CurrentBiomeWeight <= 0.f) continue;
				MtoPRatio += Preset->Biomes[I - 1].MountainRatio * CurrentBiomeWeight;
			}

			const uint16 BiomeMinHeight = static_cast<uint16>((BlurredMinHeights[Index] - ZOffset) * 128.f / ZScale + 32768.f);
			const uint16 TargetPlainHeight = FMath::Lerp(CurrentHeight, BiomeMinHeight, (1.0f - MtoPRatio) * Preset->PlainSmoothFactor);

			const float MaxAmplitude = (65535.f - TargetPlainHeight) * ZScale / HeightRange / 128.f;
			const float Amplitude = MaxAmplitude * Preset->BiomeNoiseAmplitude;
			const float DetailNoise = FMath::PerlinNoise2D(FVector2D(static_cast<float>(X), static_cast<float>(Y)) * Preset->BiomeNoiseScale) * Amplitude + Amplitude;
			const float HeightToAdd = DetailNoise * HeightRange * 128.f / ZScale;
			const float MountainHeight = FMath::Clamp(HeightToAdd + TargetPlainHeight, 0.f, 65535.f);

			uint16 NewHeight = FMath::Lerp(TargetPlainHeight, static_cast<uint16>(MountainHeight), MtoPRatio);
			NewHeight = static_cast<uint16>(FMath::Clamp(static_cast<int32>(FMath::Max(NewHeight, SeaLevelHeight)), 0, 65535));
			DataContainer.HeightMapData[Index] = NewHeight;
		}
	}
}

void UOCGLandscapeGenSubsystem::CalculateBiomeMinHeights(const UMapPreset* Preset, const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap, TArray<float>& OutMinHeights, float ZScale, float ZOffset)
{
	const FIntPoint MapSize = Preset->MapResolution;
	const int32 TotalPixels = MapSize.X * MapSize.Y;

	TArray<int32> RegionIDMap;
	RegionIDMap.Init(0, TotalPixels);
	OutMinHeights.Init(0.f, TotalPixels);

	TMap<int32, float> RegionMinHeight;
	int32 CurrentRegionID = 1;

	for (int32 Y = 0; Y < MapSize.Y; ++Y)
	{
		for (int32 X = 0; X < MapSize.X; ++X)
		{
			if (RegionIDMap[Y * MapSize.X + X] == 0)
			{
				float MinimumHeight;
				GetBiomeStats(MapSize, X, Y, CurrentRegionID, MinimumHeight, RegionIDMap, InHeightMap, InBiomeMap, ZScale, ZOffset);
				RegionMinHeight.Add(CurrentRegionID, MinimumHeight);
				++CurrentRegionID;
			}
		}
	}

	for (int32 I = 0; I < TotalPixels; ++I)
	{
		OutMinHeights[I] = RegionMinHeight.FindRef(RegionIDMap[I]);
	}
}

void UOCGLandscapeGenSubsystem::BlurBiomeMinHeights(const UMapPreset* Preset, const TArray<float>& InMinHeights, TArray<float>& OutMinHeights)
{
	const int32 BlendRadius = static_cast<int32>(Preset->BiomeHeightBlendRadius);
	const FIntPoint MapSize = Preset->MapResolution;
	const int32 TotalPixels = MapSize.X * MapSize.Y;
	OutMinHeights.SetNumUninitialized(TotalPixels);

	TArray<float> HorizontalPass;
	HorizontalPass.Init(0.f, TotalPixels);

	for (int32 Y = 0; Y < MapSize.Y; ++Y)
	{
		float Sum = 0.f;
		int32 ValidPixelCount = 0;
		for (int32 I = -BlendRadius; I <= BlendRadius; ++I)
		{
			const int32 CurrentX = FMath::Clamp(I, 0, MapSize.X - 1);
			Sum += InMinHeights[Y * MapSize.X + CurrentX];
			++ValidPixelCount;
		}
		HorizontalPass[Y * MapSize.X + 0] = ValidPixelCount > 0 ? Sum / ValidPixelCount : InMinHeights[Y * MapSize.X + 0];

		for (int32 X = 1; X < MapSize.X; ++X)
		{
			const int32 OldX = FMath::Clamp(X - BlendRadius - 1, 0, MapSize.X - 1);
			Sum -= InMinHeights[Y * MapSize.X + OldX];
			--ValidPixelCount;
			const int32 NewX = FMath::Clamp(X + BlendRadius, 0, MapSize.X - 1);
			Sum += InMinHeights[Y * MapSize.X + NewX];
			++ValidPixelCount;
			HorizontalPass[Y * MapSize.X + X] = ValidPixelCount > 0 ? Sum / ValidPixelCount : InMinHeights[Y * MapSize.X + X];
		}
	}

	for (int32 X = 0; X < MapSize.X; ++X)
	{
		float Sum = 0.f;
		int32 ValidPixelCount = 0;
		for (int32 I = -BlendRadius; I <= BlendRadius; ++I)
		{
			const int32 CurrentY = FMath::Clamp(I, 0, MapSize.Y - 1);
			Sum += HorizontalPass[CurrentY * MapSize.X + X];
			++ValidPixelCount;
		}
		OutMinHeights[0 * MapSize.X + X] = ValidPixelCount > 0 ? Sum / ValidPixelCount : HorizontalPass[0 * MapSize.X + X];

		for (int32 Y = 1; Y < MapSize.Y; ++Y)
		{
			const int32 OldY = FMath::Clamp(Y - BlendRadius - 1, 0, MapSize.Y - 1);
			Sum -= HorizontalPass[OldY * MapSize.X + X];
			--ValidPixelCount;
			const int32 NewY = FMath::Clamp(Y + BlendRadius, 0, MapSize.Y - 1);
			Sum += HorizontalPass[NewY * MapSize.X + X];
			++ValidPixelCount;
			OutMinHeights[Y * MapSize.X + X] = ValidPixelCount > 0 ? Sum / ValidPixelCount : HorizontalPass[Y * MapSize.X + X];
		}
	}
}

void UOCGLandscapeGenSubsystem::GetBiomeStats(FIntPoint MapSize, int32 X, int32 Y, int32 RegionID, float& OutMinHeight, TArray<int32>& RegionIDMap, const TArray<uint16>& InHeightMap, const TArray<const FOCGBiomeSettings*>& InBiomeMap, float ZScale, float ZOffset)
{
	TQueue<FIntPoint> Queue;
	Queue.Enqueue(FIntPoint(X, Y));

	const FOCGBiomeSettings* TargetBiome = InBiomeMap[Y * MapSize.X + X];
	RegionIDMap[Y * MapSize.X + X] = RegionID;
	OutMinHeight = FLT_MAX;

	FIntPoint CurrentPoint;
	while (Queue.Dequeue(CurrentPoint))
	{
		const uint32 CurrentIndex = CurrentPoint.Y * MapSize.X + CurrentPoint.X;
		const float CurrentHeight = (InHeightMap[CurrentIndex] - 32768.f) * ZScale / 128.f + ZOffset;
		if (CurrentHeight < OutMinHeight) OutMinHeight = CurrentHeight;

		const FIntPoint Neighbors[] =
		{
			FIntPoint(CurrentPoint.X + 1, CurrentPoint.Y), FIntPoint(CurrentPoint.X - 1, CurrentPoint.Y),
			FIntPoint(CurrentPoint.X, CurrentPoint.Y + 1), FIntPoint(CurrentPoint.X, CurrentPoint.Y - 1),
		};

		for (const FIntPoint& Neighbor : Neighbors)
		{
			if (Neighbor.X >= 0 && Neighbor.X < MapSize.X && Neighbor.Y >= 0 && Neighbor.Y < MapSize.Y)
			{
				const int32 NeighborIndex = Neighbor.Y * MapSize.X + Neighbor.X;
				if (RegionIDMap[NeighborIndex] == 0 && InBiomeMap[NeighborIndex] == TargetBiome)
				{
					RegionIDMap[NeighborIndex] = RegionID;
					Queue.Enqueue(Neighbor);
				}
			}
		}
	}
}
