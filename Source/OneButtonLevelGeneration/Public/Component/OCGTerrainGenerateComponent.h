// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Replaced by UOCGPopulationSubsystem. Scheduled for removal.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGTerrainGenerateComponent.generated.h"

class AOCGLandscapeVolume;
class AOCGLevelGenerator;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONEBUTTONLEVELGENERATION_API UOCGTerrainGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOCGTerrainGenerateComponent();

public:
	UE_DEPRECATED(5.7, "Use UOCGPopulationSubsystem::ApplyPopulation() instead.")
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateTerrainInEditor();

	UE_DEPRECATED(5.7, "Use UOCGPopulationSubsystem::ApplyPopulation() instead.")
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateTerrain(UWorld* World);

private:
	AOCGLevelGenerator* GetLevelGenerator() const;

	UPROPERTY(VisibleInstanceOnly, Category = "Cache")
	TSoftObjectPtr<AOCGLandscapeVolume> OCGVolumeAssetSoftObjectPtr;
};
