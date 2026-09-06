---
title: Limitations
description: Known limitations and issues of the One-Click Level Generator.
layout: default
tags: [limitations, known issues]
nav_order: 10
---

# Limitations

While the One-Click Level Generator aims for easy and fast level creation, it has some limitations.

- **Terrain Generation**: OCG's terrain generation relies on basic **randomness**. Complex terrain shaping or detailed adjustments must be done manually.
- **River Generation (Experimental)**: River generation is experimental. It only supports basic flow and shape, complex water networks or detailed adjustments must be done manually, and the generated result may change in future updates.
- **Biome Generation**: Biome generation only supports basic vegetation and environmental settings. It does not support complex ecosystems or detailed adjustments.
- **PCG Content**: Currently, PCG content generation is limited and only supports Static Mesh Spawning. For example, it does not support Actor placement or the placement of complex structures and detailed objects.
- **Performance**: Performance degradation may occur when generating large-scale levels. This is particularly noticeable in levels with complex terrain or many objects.
- **Experimental Features**: Some dependent plugins (like the Water Plugin) include experimental features, which can affect stability. These plugins may change in future updates.
- **Editor Only**: OCG is an editor-only tool. It generates levels in the editor and has no runtime module, so generation cannot be triggered from a packaged build. The levels it produces are ordinary levels and package normally.

## Upgrading from 1.x

`MapPreset` properties were reorganised into grouped settings sections in 2.0.0. Presets saved
with 1.x are migrated automatically the first time they are loaded, but the migration runs only
once, so **back up your presets before upgrading** and check their values in the Details panel
afterwards. See the [Changelog]({{ site.baseurl }}/changelog/) for the full list of changes.

If you encounter any bugs or issues, please report them on [GitHub Issues].

[GitHub Issues]: https://github.com/Code1133/ocg/issues
