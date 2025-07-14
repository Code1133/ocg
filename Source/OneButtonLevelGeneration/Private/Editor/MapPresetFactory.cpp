// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/MapPresetFactory.h"

#include "OCGDeveloperSettings.h"
#include "PCGGraph.h"
#include "Data/MapPreset.h"


UMapPresetFactory::UMapPresetFactory()
{
	SupportedClass = UMapPreset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UMapPresetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	UMapPreset* NewPreset = NewObject<UMapPreset>(InParent, Class, Name, Flags, Context);

	const UOCGDeveloperSettings* Settings = GetDefault<UOCGDeveloperSettings>();

	if (!Settings)
	{
		UE_LOG(LogTemp, Error, TEXT("UMapPreset::UMapPreset() Invalid OCGDeveloperSettings!"));
		return NewPreset;		
	}

	UMaterialInstance* LandscapeMaterial = Settings->DefaultLandscapeMaterialPath.LoadSynchronous();
	if (!Settings->DefaultLandscapeMaterialPath.IsNull())
	{
		if (LandscapeMaterial)
		{
			NewPreset->LandscapeMaterial = LandscapeMaterial;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UMapPreset::UMapPreset() Failed to load default landscape material!"));
		}
	}

	UPCGGraph* PCGGraph = Settings->DefaultPCGGraphPath.LoadSynchronous();
	if (!Settings->DefaultPCGGraphPath.IsNull())
	{
		if (PCGGraph)
		{
			NewPreset->PCGGraph = PCGGraph;
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UMapPreset::UMapPreset() Failed to load default PCG graph!"));
		}
	}

	return NewPreset;
}
