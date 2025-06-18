// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OCGLandscapeVolume.generated.h"

class UBoxComponent;
class UPCGComponent;


UCLASS()
class ONEBUTTONLEVELGENERATION_API AOCGLandscapeVolume : public AActor
{
	GENERATED_BODY()

public:
	AOCGLandscapeVolume();

private:
	/** PCG Box Volume */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Box", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> BoxComponent;

	/** PCG Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PCG", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPCGComponent> PCGComponent;
};
