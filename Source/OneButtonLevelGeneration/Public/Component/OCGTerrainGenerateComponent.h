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

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateTerrainInEditor();

	UFUNCTION(CallInEditor, Category = "Actions")
	void GenerateTerrain(UWorld* World);

	UFUNCTION()
	void SetTargetVolumeClass(const TSubclassOf<AOCGLandscapeVolume>& InClass) { TargetVolumeClass = InClass; }
	
private:
	AOCGLevelGenerator* GetLevelGenerator() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess="true"))
	TObjectPtr<AOCGLandscapeVolume> TargetTerrainVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess="true"))
	TSubclassOf<AOCGLandscapeVolume> TargetVolumeClass;
};
