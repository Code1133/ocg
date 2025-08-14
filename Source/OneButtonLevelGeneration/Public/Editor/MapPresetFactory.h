// Copyright (c) 2025 Code1133. All rights reserved.

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
