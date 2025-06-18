// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGLevelGenerator.generated.h"

class UOCGTerrainGenerateComponent;
class UOCGMapGenerateComponent;

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

public:
	TObjectPtr<UOCGMapGenerateComponent> GetMapGenerateComponent() { return MapGenerateComponent; }
	TObjectPtr<UOCGTerrainGenerateComponent> GetTerrainGenerateComponent() { return TerrainGenerateComponent; }

	const TArray<uint16>& GetHeightMapData() const;
	const TArray<uint16>& GetTemperatureMapData() const;
	const TArray<uint16>& GetHumidityMapData() const;

	const TMap<FName, FOCGBiomeSettings>& GetBiomes() const;

	const FIntPoint& GetMapResolution() const;
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MapGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGMapGenerateComponent> MapGenerateComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TerrainGenerator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOCGTerrainGenerateComponent> TerrainGenerateComponent;
};
