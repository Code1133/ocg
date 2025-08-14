// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/MapPresetViewportClient.h"
#include "Editor/MapPresetEditorToolkit.h"


FMapPresetViewportClient::FMapPresetViewportClient(
	[[maybe_unused]] TSharedPtr<FMapPresetEditorToolkit> InToolkit,
	UWorld* InWorld,
	const TSharedPtr<SEditorViewport>& InEditorViewportWidget
)
	: FEditorViewportClient(nullptr, nullptr, InEditorViewportWidget)
{
	SetRealtime(true);
	EngineShowFlags.SetGrid(true);
	bShowWidget = false;

	MapPresetEditorWorld = InWorld;
}

void FMapPresetViewportClient::Tick(float DeltaSeconds)
{
	FEditorViewportClient::Tick(DeltaSeconds);

	if (MapPresetEditorWorld.IsValid())
	{
		MapPresetEditorWorld->Tick(LEVELTICK_All, DeltaSeconds);
	}
}

UWorld* FMapPresetViewportClient::GetWorld() const
{
	return MapPresetEditorWorld.Get();
}
