// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGLandscapeVolume.generated.h"

class UMapPreset;
class UPCGGraph;
class UPCGComponent;
class UBoxComponent;


UCLASS()
class ONEBUTTONLEVELGENERATION_API AOCGLandscapeVolume: public AActor
{
	GENERATED_BODY()

public:
	AOCGLandscapeVolume();

	UBoxComponent* GetBoxComponent() const { return BoxComponent; }
	UPCGComponent* GetPCGComponent() const { return PCGComponent; }

	UFUNCTION(BlueprintCallable, Category = "OneClickGeneration|Utilities")
	void AdjustVolumeToBoundsOfActor(const AActor* TargetActor);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OneClickGeneration")
	const UMapPreset* MapPreset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DuplicateTransient, Category = "OneClickGeneration")
	bool bShowDebugPoints = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OneClickGeneration")
	bool bShowDebugPointsAngle = false;

#if WITH_EDITOR
	UPROPERTY(EditAnywhere, BlueprintReadOnly, DuplicateTransient, Category = "OneClickGeneration")
	bool bEditorAutoGenerate = true;
#endif

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPCGComponent> PCGComponent;
};
