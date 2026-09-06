---
title: Custom Generation Strategies
description: Replace individual stages of OCG terrain generation with your own C++ implementation.
layout: default
tags: [custom strategies, extension, c++]
nav_order: 6
---

# Custom Generation Strategies

{: .info }
> New in 2.0.0. Requires a C++ project.

OCG builds terrain in seven stages. Each one is a separate class, and you can replace any of
them with your own implementation without touching the rest of the pipeline.

## The Stages

Stages run in this order. Each writes its result into a shared data container that the next
stage reads.

| Stage | Base Class | Responsibility |
| :---- | :--------- | :------------- |
| Heightmap        | `UOCGHeightmapStrategyBase`       | Produce the initial height data |
| Temperature      | `UOCGTemperatureStrategyBase`     | Produce the temperature map |
| Humidity         | `UOCGHumidityStrategyBase`        | Produce the humidity map |
| Biome            | `UOCGBiomeStrategyBase`           | Decide and blend biomes |
| Terrain Modifier | `UOCGTerrainModifierStrategyBase` | Reshape terrain per biome |
| Erosion          | `UOCGErosionStrategyBase`         | Apply an erosion simulation |
| Smoothing        | `UOCGSmoothingStrategyBase`       | Smooth the final height data |

## Writing a Strategy

Derive from the base class for the stage you want to replace and override its virtual methods.
Most stages have a single one; the biome stage has two, `DecideAndBlendBiomes` and
`FinalizeBiomes`. This example replaces erosion with a pass that does nothing, which is a
useful starting point for confirming your class is actually being used.

```cpp
// FlatErosionStrategy.h
#pragma once

#include "CoreMinimal.h"
#include "Strategies/OCGErosionStrategyBase.h"
#include "FlatErosionStrategy.generated.h"

UCLASS()
class MYPROJECTEDITOR_API UFlatErosionStrategy : public UOCGErosionStrategyBase
{
    GENERATED_BODY()

public:
    virtual void ApplyErosion(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer) override;
};
```

```cpp
// FlatErosionStrategy.cpp
#include "FlatErosionStrategy.h"

void UFlatErosionStrategy::ApplyErosion(const UMapPreset* Preset, FOCGWorldDataContainer& DataContainer)
{
    // Leaving DataContainer.HeightMapData untouched means no erosion is applied.
}
```

Your module needs `OneButtonLevelGeneration` in its dependencies:

```csharp
PrivateDependencyModuleNames.AddRange(new string[] { "OneButtonLevelGeneration" });
```

{: .warning }
> `OneButtonLevelGeneration` is an **editor-only** module. Put your strategy classes in an
> editor module of your own. A runtime module that links against it will fail to package.

## Selecting a Strategy

Open **Edit → Project Settings → Plugins → One Button Level Generation Settings →
Generation Strategies** and pick your class for the stage you replaced.

The setting is project-wide — it is stored in `Config/DefaultOneButtonLevelGeneration.ini` and
applies to every `MapPreset`. A preset cannot override it.

{: .note }
> The subsystem creates its strategies once, when the editor starts. **Restart the editor after
> changing a strategy class**, otherwise the previous one stays in use for the rest of the
> session.

Leaving a stage empty falls back to the built-in implementation silently — that is the normal
default state. Selecting an abstract class also falls back, but writes a warning to the Output
Log. A bad strategy setting never stops generation.

## Blueprint Is Not Supported

Strategies must be written in C++. The `FOCGWorldDataContainer` they read and write is a plain
C++ struct holding `TArray<uint16>` and `TMap<FName, TArray<uint8>>` — types Blueprint cannot
represent. Exposing it would mean widening the height data to `int32`, doubling memory on maps
that routinely hold several million samples.

## Verifying Your Strategy Is Used

The no-op erosion example above is built for exactly this check: select it, restart the editor,
generate with a preset that has erosion enabled, and confirm the terrain comes out uneroded. If
erosion still appears, your class is not being used — check that you restarted the editor, and
that the class is actually selected in Project Settings. An empty slot falls back without
logging anything, so no message in the Output Log does not mean the setting took effect.
