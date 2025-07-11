// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGLevelGenerator.generated.h"

class ALandscape;
struct FOCGBiomeSettings;
class UOCGLandscapeGenerateComponent;
class UOCGTerrainGenerateComponent;
class UOCGMapGenerateComponent;
class UOCGRiverGenerateComponent;
class AStaticMeshActor;
class UMapPreset;

UCLASS()
class ONEBUTTONLEVELGENERATION_API AOCGLevelGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AOCGLevelGenerator();
	
	UFUNCTION(CallInEditor, Category = "Actions")
	void Generate();
	
	UFUNCTION(CallInEditor, Category = "Actions")
	void OnClickGenerate(UWorld* InWorld);
	
	UOCGMapGenerateComponent* GetMapGenerateComponent() { return MapGenerateComponent; }
	UOCGTerrainGenerateComponent* GetTerrainGenerateComponent() { return TerrainGenerateComponent; }
	UOCGLandscapeGenerateComponent* GetLandscapeGenerateComponent() { return LandscapeGenerateComponent; }
	UOCGRiverGenerateComponent* GetRiverGenerateComponent() { return RiverGenerateComponent; }

	const TArray<uint16>& GetHeightMapData() const;
	const TArray<uint16>& GetTemperatureMapData() const;
	const TArray<uint16>& GetHumidityMapData() const;
	const TMap<FName, TArray<uint8>>& GetWeightLayers() const;
	const ALandscape* GetLandscape() const;
	ALandscape* GetLandscape();
	FVector GetVolumeExtent() const;
	FVector GetVolumeOrigin() const;

	void SetMapPreset(UMapPreset* InMapPreset);

	const UMapPreset* GetMapPreset() const { return MapPreset; }
	UMapPreset* GetMapPreset() { return MapPreset; }

	void AddWaterPlane(UWorld* InWorld);
	void SetDefaultWaterProperties(class AWaterBody* InWaterBody);
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMapPreset> MapPreset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LevelGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class AWaterBodyOcean> SeaLevelWaterBody;
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGMapGenerateComponent> MapGenerateComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LandsacpeGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGLandscapeGenerateComponent> LandscapeGenerateComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGTerrainGenerateComponent> TerrainGenerateComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RiverGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGRiverGenerateComponent> RiverGenerateComponent;
};
