// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/OCGBiomeSettings.h"
#include "OCGComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONEBUTTONLEVELGENERATION_API
UOCGComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UOCGComponent();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	TArray<FOCGBiomeSettings> Biomes; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "OCG")
	UMaterialInstance* LandscapeMaterialInstance;
};
