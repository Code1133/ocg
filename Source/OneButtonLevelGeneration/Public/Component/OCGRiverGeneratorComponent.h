// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGRiverGeneratorComponent.generated.h"

class UMapPreset;
class AWaterBodyRiver;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONEBUTTONLEVELGENERATION_API UOCGRiverGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UOCGRiverGeneratorComponent();

	UFUNCTION(CallInEditor, Category = "River Generation")
	void GenerateRiver(UWorld* InWorld, class ALandscape* InLandscape);

	void SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset, float InMinHeight, float InMaxHeight);

	void SetRiverWidth(AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath);

private:
	FVector GetLandscapePointWorldPosition(const FIntPoint& MapPoint, const FVector& LandscapeOrigin, const FVector& LandscapeExtent) const;
	
	void SetDefaultRiverProperties(class AWaterBodyRiver* InRiverActor, const TArray<FVector>& InRiverPath);
	// helper functions
	FIntPoint GetRandomStartPoint(float StartPointThresholdMultiplier, ALandscape* InLandscape);

	void SimplifyPathRDP(const TArray<FVector>& InPoints, TArray<FVector>& OutPoints, float Epsilon);

	UPROPERTY(Transient)
	TObjectPtr<UMapPreset> MapPreset;

	TArray<uint16> HeightMapData;
	float SeaHeight;

	// Minimum and Maximum height of the current landscape
	float MinHeight;
	float MaxHeight;

	// Generated rivers
	UPROPERTY(Transient)
	TArray<AWaterBodyRiver*> GeneratedRivers;

	UPROPERTY(Transient)
	TObjectPtr<ALandscape> TargetLandscape;
	
	TSet<FIntPoint> UsedRiverStartPoints;
};



