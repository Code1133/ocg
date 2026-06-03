// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Factories/MapPresetFactory.h"

#include "OCGDeveloperSettings.h"
#include "OCGLog.h"
#include "PCGGraph.h"
#include "Data/MapPreset.h"

UMapPresetFactory::UMapPresetFactory()
{
	SupportedClass = UMapPreset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMapPresetFactory::FactoryCreateNew(
	UClass* Class, UObject* InParent, FName Name,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn
)
{
	UMapPreset* NewPreset = NewObject<UMapPreset>(InParent, Class, Name, Flags, Context);

	const UOCGDeveloperSettings* Settings = GetDefault<UOCGDeveloperSettings>();
	if (!Settings)
	{
		UE_LOG(LogOCGModule, Error, TEXT("MapPresetFactory: Invalid OCGDeveloperSettings."));
		return NewPreset;
	}

	if (!Settings->DefaultLandscapeMaterialPath.IsNull())
	{
		if (UMaterialInstance* Mat = Settings->DefaultLandscapeMaterialPath.LoadSynchronous())
		{
			NewPreset->LandscapeMaterial = Mat;
		}
		else
		{
			UE_LOG(LogOCGModule, Warning, TEXT("MapPresetFactory: Failed to load default landscape material."));
		}
	}

	if (!Settings->DefaultPCGGraphPath.IsNull())
	{
		if (UPCGGraph* Graph = Settings->DefaultPCGGraphPath.LoadSynchronous())
		{
			NewPreset->PCGGraph = Graph;
		}
		else
		{
			UE_LOG(LogOCGModule, Warning, TEXT("MapPresetFactory: Failed to load default PCG graph."));
		}
	}

	return NewPreset;
}
