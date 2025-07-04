// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGRiverGeneratorComponent.generated.h"

class UMapPreset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONEBUTTONLEVELGENERATION_API UOCGRiverGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UOCGRiverGeneratorComponent();

	UFUNCTION(CallInEditor, Category = "River Generation")
	void GenerateRiver(UWorld* InWorld, class ALandscape* InLandscape);

	void SetMapData(const TArray<uint16>& InHeightMap, UMapPreset* InMapPreset);

private:
	void SpawnWaterRiver();
	FVector GetWorldLocation(const FIntPoint& MapPoint, class ALandscape* InLandscape) const;

	// helper functions
	FIntPoint GetRandomStartPoint(float StartPointThresholdMultiplier);

	UPROPERTY()
	class USplineComponent* RiverSpline;

	UPROPERTY(Transient)
	TObjectPtr<UMapPreset> MapPreset;
	
	TArray<uint16> HeightMapData;
	float SeaHeight;
};
