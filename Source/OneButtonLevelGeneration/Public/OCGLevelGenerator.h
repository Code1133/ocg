// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGLevelGenerator.generated.h"

class ALandscape;
struct FOCGBiomeSettings;
class UOCGLandscapeGenerateComponent;
class UOCGTerrainGenerateComponent;
class UOCGMapGenerateComponent;
class AStaticMeshActor;
class UMapPreset;

UCLASS()
class ONEBUTTONLEVELGENERATION_API AOCGLevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOCGLevelGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(CallInEditor, Category = "Actions")
	void Generate();
	
	UFUNCTION(CallInEditor, Category = "Actions")
	void OnClickGenerate(UWorld* InWorld);
	
	UOCGMapGenerateComponent* GetMapGenerateComponent() { return MapGenerateComponent; }
	UOCGTerrainGenerateComponent* GetTerrainGenerateComponent() { return TerrainGenerateComponent; }

	const TArray<uint16>& GetHeightMapData() const;
	const TArray<uint16>& GetTemperatureMapData() const;
	const TArray<uint16>& GetHumidityMapData() const;
	const TMap<FName, TArray<uint8>>& GetWeightLayers() const;
	const ALandscape* GetLandscape() const;

	void SetMapPreset(UMapPreset* InMapPreset);

	const UMapPreset* GetMapPreset() const { return MapPreset; }

	void AddWaterPlane(UWorld* InWorld);
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMapPreset> MapPreset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AStaticMeshActor> PlaneActor;
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGMapGenerateComponent> MapGenerateComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandsacpeGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGLandscapeGenerateComponent> LandscapeGenerateComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGTerrainGenerateComponent> TerrainGenerateComponent;
};