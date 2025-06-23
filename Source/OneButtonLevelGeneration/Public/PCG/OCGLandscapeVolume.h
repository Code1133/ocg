// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGLandscapeVolume.generated.h"

class UPCGGraph;
class UOCGHierarchyDataAsset;
class UPCGComponent;
class UBoxComponent;


UCLASS(Abstract, Blueprintable)
class ONEBUTTONLEVELGENERATION_API AOCGLandscapeVolume : public AActor
{
	GENERATED_BODY()

public:
	AOCGLandscapeVolume();

	UBoxComponent* GetBoxComponent() const { return BoxComponent; }
	UPCGComponent* GetPCGComponent() const { return PCGComponent; }

	UFUNCTION(BlueprintCallable, Category = "OneClickGeneration|Utilities")
	void AdjustVolumeToBoundsOfActor(const AActor* TargetActor);

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OneClickGeneration")
	TObjectPtr<UOCGHierarchyDataAsset> DataAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OneClickGeneration")
	bool bShowDebugPoints = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPCGComponent> PCGComponent;
};
