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

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OneClickGeneration")
	const UMapPreset* MapPreset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DuplicateTransient, Category = "OneClickGeneration")
	bool bShowDebugPoints = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, DuplicateTransient, Category = "OneClickGeneration")
	bool bEditorAutoGenerate = true;

	void SetEditorAutoGenerate(bool bEnable);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoxComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPCGComponent> PCGComponent;
};
