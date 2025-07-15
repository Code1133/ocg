// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGRiverGeneratorComponent.generated.h"

class AOCGLevelGenerator;
class UMapPreset;
class AWaterBodyRiver;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONEBUTTONLEVELGENERATION_API UOCGRiverGenerateComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UOCGRiverGenerateComponent();

	UFUNCTION(CallInEditor, Category = "River Generation")
	void GenerateRiver(UWorld* InWorld, class ALandscape* InLandscape);

	void SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset, float InMinHeight, float InMaxHeight);

	void AddRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath);

	AOCGLevelGenerator* GetLevelGenerator() const;

	UFUNCTION(CallInEditor, Category = "Actions")
	void ApplyWaterWeight();
private:
	void ExportWaterEditLayerHeightMap();
	
	void ClearAllRivers();

	FVector GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const;
	
	void SetDefaultRiverProperties(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath);
	// helper functions
	FIntPoint GetRandomStartPoint();

	void SimplifyPathRDP(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints, float Epsilon);

	void CacheRiverStartPoints();

	UPROPERTY(Transient)
	TObjectPtr<UMapPreset> MapPreset;
	
	float SeaHeight = 0.0f;

	// Generated rivers
	UPROPERTY(Transient)
	TArray<AWaterBodyRiver*> GeneratedRivers;

	UPROPERTY(VisibleInstanceOnly, Category = "Cache")
	TArray<TSoftObjectPtr<AWaterBodyRiver>> CachedRivers;

	UPROPERTY(Transient)
	TObjectPtr<ALandscape> TargetLandscape;
	
	TSet<FIntPoint> UsedRiverStartPoints;
	TArray<FIntPoint> CachedRiverStartPoints;

	TArray<uint16> CachedRiverHeightMap;

	TArray<uint8> PrevWaterWeightMap;

	uint16 RiverHeightMapWidth;
	uint16 RiverHeightMapHeight;
};



