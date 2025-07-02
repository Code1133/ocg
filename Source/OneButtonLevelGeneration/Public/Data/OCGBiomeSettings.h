#pragma once

#include "OCGBiomeSettings.generated.h"

USTRUCT(BlueprintType)
struct FOCGBiomeStaticMesh
{
    GENERATED_BODY()
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    UStaticMesh* StaticMesh;

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
	float Temperature;

	//Sets Biomes standard Humidity (value between 0~1)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Humidity;

	//디버그 용 후에 삭제 예정
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
	FLinearColor Color;

	//나중에 사용할수도?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    int Weight = 1;

	//Sets the terrain feature of this Biome (0 = Plain, 1 = Mountain)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MountainRatio = 0.5f;
};
