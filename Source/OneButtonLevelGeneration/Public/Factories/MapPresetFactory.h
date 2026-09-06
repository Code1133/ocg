// Copyright (c) 2025-2026 Code1133. All rights reserved.
#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MapPresetFactory.generated.h"

/**
 * UFactory that creates a new UMapPreset asset from the Content Browser.
 * Applies defaults from UOCGDeveloperSettings (LandscapeMaterial, PCGGraph).
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UMapPresetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UMapPresetFactory();

	virtual UObject* FactoryCreateNew(
		UClass* Class, UObject* InParent, FName Name,
		EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
	) override;
};
