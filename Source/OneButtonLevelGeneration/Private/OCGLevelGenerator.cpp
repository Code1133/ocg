// Fill out your copyright notice in the Description page of Project Settings.


#include "OCGLevelGenerator.h"
#include "Component/OCGMapGenerateComponent.h"
#include "Component/OCGLandscapeGenerateComponent.h"
#include "Component/OCGTerrainGenerateComponent.h"

// Sets default values
AOCGLevelGenerator::AOCGLevelGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AOCGLevelGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AOCGLevelGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

const TArray<uint16>& AOCGLevelGenerator::GetHeightMapData() const
{
	return MapGenerateComponent->GetHeightMapData();
}

const TArray<uint16>& AOCGLevelGenerator::GetTemperatureMapData() const
{
	return MapGenerateComponent->GetTemperatureMapData();
}

const TArray<uint16>& AOCGLevelGenerator::GetHumidityMapData() const
{
	return MapGenerateComponent->GetHumidityMapData();
}

const TMap<FName, FOCGBiomeSettings>& AOCGLevelGenerator::GetBiomes() const
{
	return MapGenerateComponent->GetBiomes();
}

const FIntPoint& AOCGLevelGenerator::GetMapResolution() const
{
	return MapGenerateComponent->GetMapResolution();
}

