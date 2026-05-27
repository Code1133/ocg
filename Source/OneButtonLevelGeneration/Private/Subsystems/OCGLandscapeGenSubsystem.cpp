// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGLandscapeGenSubsystem.h"

#include "OCGLog.h"
#include "OCGStats.h"
#include "Data/MapPreset.h"
#include "Data/OCGWorldDataContainer.h"
#include "Utils/OCGLandscapeUtil.h"

#include "Editor.h"
#include "EngineUtils.h"
#include "Engine/Level.h"

#include "Landscape.h"
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 6
	#include "LandscapeEditLayer.h"
#endif
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "LandscapeStreamingProxy.h"

#include "LocationVolume.h"
#include "RuntimeVirtualTextureSetBounds.h"
#include "Components/BoxComponent.h"
#include "Components/RuntimeVirtualTextureComponent.h"
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
	SCOPE_CYCLE_COUNTER(STAT_OCG_LandscapeModify);

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
		if (ULevel* CurrentLevel = World->GetCurrentLevel())
		{
			CurrentLevel->MarkPackageDirty();
		}
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
		if (ULevel* Level = NewRVTVolume->GetLevel())
		{
			Level->MarkPackageDirty();
		}
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
