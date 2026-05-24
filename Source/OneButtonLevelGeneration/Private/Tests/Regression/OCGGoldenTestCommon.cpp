// Copyright (c) 2025-2026 Code1133. All rights reserved.
#include "Tests/Regression/OCGGoldenTestCommon.h"

#include "Data/MapPreset.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "OCGLevelGenerator.h"

FOCGGoldenFixture::FOCGGoldenFixture()
	: World(nullptr)
	, Generator(nullptr)
{
	World = UWorld::CreateWorld(EWorldType::GamePreview, false);
	FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::GamePreview);
	Ctx.SetCurrentWorld(World);
	World->InitializeActorsForPlay(FURL());
	Generator = World->SpawnActor<AOCGLevelGenerator>();
}

FOCGGoldenFixture::~FOCGGoldenFixture()
{
	if (Generator)
	{
		Generator->Destroy();
	}
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
}

bool FOCGGoldenFixture::IsValid() const
{
	return Generator != nullptr;
}

UMapPreset* TryLoadPreset(FAutomationTestBase* Test, const TCHAR* AssetPath)
{
	UMapPreset* Preset = LoadObject<UMapPreset>(nullptr, AssetPath);
	if (!Preset)
	{
		Test->AddWarning(FString::Printf(TEXT("Preset not found, skipping: %s"), AssetPath));
	}
	return Preset;
}

bool CheckHeightMapCRC(FAutomationTestBase* Test, TArrayView<const uint16> HeightMapData, uint32 ExpectedCRC, const TCHAR* PresetName)
{
	const uint32 ActualCRC = FCrc::MemCrc32(HeightMapData.GetData(), HeightMapData.Num() * sizeof(uint16));

	if (ExpectedCRC == 0)
	{
		Test->AddInfo(FString::Printf(TEXT("[RECORD] %s HeightMap CRC32 = 0x%08X — paste into ExpectedCRC"), PresetName, ActualCRC));
		return true;
	}

	if (ActualCRC != ExpectedCRC)
	{
		Test->AddError(FString::Printf(TEXT("%s HeightMap CRC mismatch: expected 0x%08X, got 0x%08X"), PresetName, ExpectedCRC, ActualCRC));
		return false;
	}
	return true;
}
