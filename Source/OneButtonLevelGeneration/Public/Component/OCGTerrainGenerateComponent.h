// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGTerrainGenerateComponent.generated.h"

class AOCGLandscapeVolume;
class UOCGHierarchyDataAsset;
class AOCGLevelGenerator;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONEBUTTONLEVELGENERATION_API UOCGTerrainGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOCGTerrainGenerateComponent();

public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateTerrainInEditor();

	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateTerrain(UWorld* World);

private:
	AOCGLevelGenerator* GetLevelGenerator() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG", meta = (AllowPrivateAccess="true"))
	TObjectPtr<AOCGLandscapeVolume> OCGVolumeInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG", meta = (AllowPrivateAccess="true"))
	TSubclassOf<AOCGLandscapeVolume> TargetVolumeClass;
};
