// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "OCGBiomeSettings.generated.h"

USTRUCT(BlueprintType)
struct FOCGBiomeStaticMesh
{
    GENERATED_BODY()
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    UStaticMesh* StaticMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    int Weight = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    FVector Scale = FVector(1.0f);
};

USTRUCT(BlueprintType)
struct FOCGBiomeSettings
{
    GENERATED_BODY()

	//Sets Biome Name
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    FName BiomeName = TEXT("LayerName");

	//Sets Biomes standard Temperature (degree celsius)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	float Temperature = 15.0f;

	//Sets Biomes standard Humidity (value between 0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Humidity = 0.5f;

	//Sets the color of the biome in exported biome map
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FLinearColor Color = FLinearColor::Black;

	//The weight of the biome in the overall generation process. Higher weights increase the likelihood of this biome being selected.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0"))
    int Weight = 1;

	//Sets the terrain feature of this Biome (0 = Plain, 1 = Mountain)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MountainRatio = 0.5f;
};
