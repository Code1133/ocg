// Copyright (c) 2025 Code1133. All rights reserved.
// [DEPRECATED v2] Custom MapPreset editor. Replaced by default property editor. Scheduled for removal.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MapPresetFactory.generated.h"

/**
 * 
 */
UCLASS()
class ONEBUTTONLEVELGENERATION_API UMapPresetFactory : public UFactory
{
	GENERATED_BODY()
	
public:
	UMapPresetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
