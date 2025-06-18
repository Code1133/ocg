// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OCGBiomeSettings.h" 
#include "MapPreset.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, meta = (DisplayName = "Map Preset"))
class ONEBUTTONLEVELGENERATION_API UMapPreset : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	uint8 MapSize = 63;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	uint8 bUseOwnMaterMaterial = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	TArray<FOCGBiomeSettings> Biomes;
};
