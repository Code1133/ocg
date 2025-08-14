// Copyright (c) 2025 Code1133. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "OCGDeveloperSettings.generated.h"

class UPCGGraph;


/**
 * 
 */
UCLASS(config = OneButtonLevelGeneration, DefaultConfig, meta = (DisplayName = "One Button Level Generation Settings"))
class ONEBUTTONLEVELGENERATION_API UOCGDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	/** Default material for landscape generation */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Asset References", meta = (AllowClasses = "/Script/Engine.MaterialInstanceConstant"))
	TSoftObjectPtr<UMaterialInstance> DefaultLandscapeMaterialPath;

	/** Default PCG Graph for level generation */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Asset References", meta = (AllowClasses = "/Script/PCG.PCGGraph"))
	TSoftObjectPtr<UPCGGraph> DefaultPCGGraphPath;
};
