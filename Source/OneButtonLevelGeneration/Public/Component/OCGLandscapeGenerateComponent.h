// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGLandscapeGenerateComponent.generated.h"

class AOCGLevelGenerator;
struct FOCGBiomeSettings;
struct FLandscapeImportLayerInfo;
class ALandscape;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ONEBUTTONLEVELGENERATION_API UOCGLandscapeGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOCGLandscapeGenerateComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material", meta = (AllowPrivateAccess="true"))
	UMaterialInstance* LandscapeMaterial;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Landscape", meta = (AllowPrivateAccess="true"))
	ALandscape* TargetLandscape;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionGridSize = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite,  Category = "Landscape",  meta = (ClampMin = 4, ClampMax = 64, UIMin = 4, UIMax = 64, AllowPrivateAccess="true"))
	int32 WorldPartitionRegionSize = 16;
public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscapeInEditor();
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateLandscape(UWorld* World);
	
private:
	void FinalizeLayerInfos(ALandscape* Landscape, const TMap<FGuid, TArray<FLandscapeImportLayerInfo>>& MaterialLayerDataPerLayers);

	AOCGLevelGenerator* GetLevelGenerator() const;
private:
	float CachedGlobalMinTemp;
	float CachedGlobalMaxTemp;
	float CachedGlobalMinHumidity;
	float CachedGlobalMaxHumidity;
};
