// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Subsystems/OCGPopulationSubsystem.h"

#include "OCGLog.h"
#include "OCGStats.h"
#include "Data/MapPreset.h"
#include "PCG/OCGLandscapeVolume.h"
#include "Subsystems/OCGLandscapeGenSubsystem.h"

#include "Editor.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"

void UOCGPopulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UOCGPopulationSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UOCGPopulationSubsystem::ApplyPopulation(const UMapPreset* Preset)
{
	SCOPE_CYCLE_COUNTER(STAT_OCG_PopulationFoliage);

	if (!Preset)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("ApplyPopulation: Preset is null."));
		return;
	}

	UOCGLandscapeGenSubsystem* LandscapeSubsystem = GEditor->GetEditorSubsystem<UOCGLandscapeGenSubsystem>();
	if (!LandscapeSubsystem)
	{
		UE_LOG(LogOCGModule, Warning, TEXT("ApplyPopulation: OCGLandscapeGenSubsystem not found."));
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return;
	}

	// 캐시된 볼륨 인스턴스 복원
	AOCGLandscapeVolume* VolumeInstance = nullptr;
	if (CachedVolumeAsset.ToSoftObjectPath().IsValid())
	{
		VolumeInstance = CachedVolumeAsset.IsValid()
			? CachedVolumeAsset.Get()
			: Cast<AOCGLandscapeVolume>(CachedVolumeAsset.LoadSynchronous());
	}

	// 캐시된 인스턴스 이외의 볼륨 제거
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AOCGLandscapeVolume::StaticClass(), FoundActors);

	bool bChanged = false;
	for (AActor* Actor : FoundActors)
	{
		if (Actor != VolumeInstance)
		{
			Actor->Modify();
			World->EditorDestroyActor(Actor, true);
			bChanged = true;
		}
	}

	// 유효한 인스턴스 없으면 스폰
	if (!IsValid(VolumeInstance))
	{
		VolumeInstance = World->SpawnActor<AOCGLandscapeVolume>();
		if (!VolumeInstance)
		{
			UE_LOG(LogOCGModule, Warning, TEXT("ApplyPopulation: Failed to spawn AOCGLandscapeVolume."));
			return;
		}
		VolumeInstance->SetIsSpatiallyLoaded(false);
		VolumeInstance->Modify();
		bChanged = true;
	}

	CachedVolumeAsset = TSoftObjectPtr<AOCGLandscapeVolume>(VolumeInstance);

	VolumeInstance->SetActorLocation(LandscapeSubsystem->GetVolumeOrigin());
	VolumeInstance->GetBoxComponent()->SetBoxExtent(LandscapeSubsystem->GetVolumeExtent());
	VolumeInstance->MapPreset = Preset;

	if (UPCGGraph* PCGGraph = Preset->PCGGraph)
	{
		if (UPCGComponent* PCGComponent = VolumeInstance->GetPCGComponent())
		{
			PCGComponent->SetGraph(PCGGraph);
			if (Preset->bAutoGenerate)
			{
				PCGComponent->Generate(true);
			}
		}
	}

	if (bChanged && World->GetCurrentLevel())
	{
		(void)World->GetCurrentLevel()->MarkPackageDirty();
	}
}
