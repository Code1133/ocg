// Copyright (c) 2025 Code1133. All rights reserved.

#include "Editor/SMapPresetViewport.h"
#include "Editor/MapPresetViewportClient.h"


void SMapPresetViewport::Construct(const FArguments& InArgs)
{
	ToolkitPtr = InArgs._MapPresetEditorToolkit;
	MapPresetEditorWorld = InArgs._World;

	SEditorViewport::Construct(SEditorViewport::FArguments());
}

SMapPresetViewport::~SMapPresetViewport()
{
}

UWorld* SMapPresetViewport::GetWorld() const
{
	return MapPresetEditorWorld.Get();
}

TSharedRef<FEditorViewportClient> SMapPresetViewport::MakeEditorViewportClient()
{
	// Create viewport client
	ViewportClient = MakeShared<FMapPresetViewportClient>(ToolkitPtr.Pin(), MapPresetEditorWorld.Get(), SharedThis(this));
	return ViewportClient.ToSharedRef();
}
