// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OCGRiverGeneratorComponent.generated.h"

class UMapPreset;
class AWaterBodyRiver;

struct FCompareNode
{
	bool operator()(const TTuple<FIntPoint, float>& A, const TTuple<FIntPoint, float>& B) const
	{
		// B의 우선순위가 더 낮으면(값이 크면) A가 앞으로 가야 하므로 true를 반환합니다.
		// 즉, A의 우선순위 값이 더 작을 때 true가 됩니다.
		return A.Get<1>() < B.Get<1>();
	}
};

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
	
	TSet<FIntPoint> UsedRiverStartPoints;
};



