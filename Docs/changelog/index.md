---
title: CHANGELOG
layout: home
description: One-Click Level Generator Changelog
tags: [one-click level generator, changelog]
nav_order: 20
---

# CHANGELOG

This page documents the full version history and updates for the **One-Click Level Generator (OCG)**.

---

## Version 2.0.0-beta.1 *(2026-09-06)*

{: .warning }
> **This is a pre-release.** It is published for evaluation and feedback, and is not
> distributed through FAB. Use it on a copy of your project, not on production work.

A major internal rework. The generation pipeline moved from actor components to editor
subsystems, `MapPreset` properties were reorganised into grouped settings sections, and each
stage of terrain generation became a replaceable strategy.

### Upgrade Notes

{: .warning }
> **Back up your `MapPreset` assets before upgrading.**
> Presets saved with 1.x are migrated automatically the first time they are loaded, but the
> migration only runs once. Verify your values in the Details panel and re-save each preset
> after upgrading.

- **`MapPreset` properties are now grouped.** Fields that used to sit flat on the asset are
  organised into sections such as *Landscape Settings*, *Height Settings*, *Noise Settings*,
  *Erosion Settings*, *Temperature Settings*, *Ocean Settings* and *River Settings*.
  Values carry over automatically; only their location in the Details panel changes.
- **Default asset paths moved to Project Settings.** The landscape material, PCG graph,
  Runtime Virtual Textures and the generated LayerInfo output folder are now configured under
  **Project Settings → Plugins → One Button Level Generation Settings** instead of being
  compiled into the plugin. See [Additional Settings]({{ site.baseurl }}/additional_settings/).
- **Water materials left empty now fall back to the Water plugin defaults.** An empty material
  slot on a `MapPreset` is normal and resolves to the engine default at generation time.
- **River generation is now labelled Experimental.** Behaviour is unchanged, but the feature is
  explicitly marked in the editor and may change in future updates.

### New Features

- **Custom generation strategies.** Each of the seven terrain generation stages — heightmap,
  temperature, humidity, biome, terrain modifier, erosion and smoothing — can now be replaced
  with your own C++ implementation. Derive from the stage's base class and select it under
  **Project Settings → Plugins → One Button Level Generation Settings → Generation Strategies**.
  See [Custom Generation Strategies]({{ site.baseurl }}/custom_strategies/).
- Added **Project Settings → Plugins → One Button Level Generation Settings** for configuring
  default assets and the LayerInfo output folder, with startup validation that reports any
  configured asset that no longer resolves.
- Added a **sidebar to the OCG window** that filters the panel by category.
- Added an **OCG button to the level editor toolbar**. The window is still available from
  **Window → OCG Tools → OCG** as before.
- Rivers now have their own **Water HLOD Material** and **Underwater Post Process Material**,
  so a river-only preset can override them independently of the ocean.
- Added an **Experimental notice** in the Water tab when river generation is enabled.

### Improvements

- Rewrote the generation pipeline as editor subsystems with per-stage strategies
  (heightmap, temperature, humidity, biome, smoothing, erosion), making each stage
  independently replaceable.
- Added profiling stat groups and cycle counters to the generation stages, visible through
  `stat OCG`.
- Golden regression tests now cover heightmap output for five presets and fail loudly when a
  fixture asset is missing.

### Fixed Bugs

- Fixed water materials being dropped during generation, which left water bodies with no
  material assigned.
- Fixed **erosion carving the wrong rows on non-square maps** — when *Map Resolution* X and Y
  differed, the lower part of the terrain was left uneroded.
- Fixed the editor crashing with a fatal world-leak assertion when creating or opening a new
  level after generating.
- Fixed generated water bodies missing their **Water Info Material**, which broke water
  rendering features that depend on it.
- Fixed the ocean not rendering after generation, and fixed a mismatch between the actual sea
  level and the height used by the landscape material's blend layer.
- Fixed river properties falling back to ocean defaults instead of the river defaults.
- Fixed Details panel handlers not firing for grouped sub-properties, which stopped
  *Map Resolution* and heightmap import fields from updating linked values.
- Fixed an incorrect divisor in the landscape world-location calculation.
- Fixed generated landscape, Runtime Virtual Texture, river and ocean actors not marking the
  level package dirty, so the level could be closed without being prompted to save.

---

## Version 1.2.1 *(2026-08-01)*

### Improvements & Fixes

- Added support for **Unreal Engine 5.8**.
- Refactored internal tab factory registration logic for UE 5.8 private API changes.

---

## Version 1.2.0

### New Features

- Added support for Unreal Engine 5.7 and 5.7.1

### Fixed Bugs

- Fixed landscape generation issues in Open World levels on Unreal Engine 5.7.1
- Added compatibility for Unreal Engine 5.6 and earlier in landscape utility methods
- Added compatibility for Unreal Engine 5.6 in river generation logic

---

## Version 1.1.1

### New Features

- Added a button to the OCG Window that assigns a new River Seed and generates a river when clicked.

### Fixed Bugs

- Fixed a bug where multiple rivers would be generated at the same location when creating more than two rivers.

---

## Version 1.1.0

### New Features

- Added a feature allowing users to import custom Height Maps to generate terrain.
- Introduced a seed parameter in the river generation logic, ensuring rivers are generated in the same locations for identical seeds.
- Added a layer to prevent PCG content from being generated during level creation, which can be applied via painting in landscape mode.
- Added a Preview Map feature that displays a debug line preview of terrain generated by the current OCG seed.
- Added a Height restriction property for meshes in PCG settings.
- Added an icon to the OCG Window button, accessible from the top Window tab.
- Enhanced the UI to show the progress and tasks being performed at each stage of level generation.
- Added several sample vegetation meshes.
- These meshes now spawn when creating levels using MP_StylizedLandscape.
- Updated the logic for specifying landscape scale, enabling direct input in kilometers.

### Fixed Bugs

- Fixed a crash that occurred when the biome name in OCG settings was set to "None."
- Resolved an issue where regenerating rivers caused irrevocable loss of layer data at biome boundaries.
- Fixed a problem where, with both the OCG Window and MapPreset Editor open, rivers were only generated in the editor's world.
- Fixed an issue where debug points would not appear even when enabled in PCG settings.
- Fixed inconsistent terrain generation with the same seed during initial creation in the OCG Window.
- Fixed a crash that occurred when clearing a MapPreset in the OCG Window.

---

## Version 1.0.0

- Initial release 🎉
- Added a custom editor and the ability to create MapPreset Assets.
- Added random terrain generation.
- Added random river generation.
- Added biome-specific PCG content generation.
- Miscellaneous feature improvements and bug fixes.
