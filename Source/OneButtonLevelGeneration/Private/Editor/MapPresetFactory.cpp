// Fill out your copyright notice in the Description page of Project Settings.


#include "Editor/MapPresetFactory.h"
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
	return NewObject<UMapPreset>(InParent, Class, Name, Flags, Context);
}
