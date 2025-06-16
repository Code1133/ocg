#pragma once

#include "OCGBiomeSettings.generated.h"

USTRUCT(BlueprintType)
struct FOCGBiomeStaticMesh
{
    GENERATED_BODY()
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    UStaticMesh* StaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    int Weight = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    FVector Scale = FVector(1.0f);
};

USTRUCT(BlueprintType)
struct FOCGBiomeSettings
{
    GENERATED_BODY()
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    FString BiomeName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    float MinHeight = -100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    float MaxHeight = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    UMaterialInterface* BiomeMaterial;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
	UMaterialFunctionInterface* BiomeMaterialFunc;
	
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    TArray<FOCGBiomeStaticMesh> StaticMeshSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome", meta = (AllowPrivate))
    int Weight = 1;
};
