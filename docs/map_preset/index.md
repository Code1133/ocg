---
title: MapPreset
description: Data asset that is core to level creation
layout: default
tags: [map_preset, asset]
nav_order: 4
has_children: true
---

# MapPreset

The **MapPreset** is the core data asset of the *OCG plugin*.
By editing a **MapPreset**, you can define properties related to level generation, such as *biome-specific vegetation*, *terrain appearance*, and *river generation*.

![MapPreset asset image]({{ site.baseurl }}/assets/images/map_preset/map_preset.png)

- You can create a new **MapPreset** from the **OCG Window** or by right-clicking in the **Content Drawer** and selecting **OCG > Map Preset**.


## Editing MapPreset
You can edit the **MapPreset** asset and generate a level from the **OCG Window** or the custom **MapPreset editor**.

![OCGWindow]({{ site.baseurl }}/assets/images/map_preset/OCGWindow.png)

| Button | Description |
| Create Level Generator / Select Level Generator | Create New ALevelGenerator in current Level. / Select one ALevelGenertor in current Level.                                                                                                          |
| Preview Map | The Landscape that will be generated based on the .png specified in the HeightMap File Path is previewed with a green outline and maps such as temperature maps are generated as png file in Contents/Maps folder. If there is no HeightMap File Path, the Landscape generated from the current Seed value is previewed with a green outline.                                                                                                          |
| Generate | Generate Landscape & PCG & Rivers based on the MapPreset in current Level.                                                                                                          |
| Generate With Random Seed | Generate Landscape & PCG & Rivers based on the MapPreset with Random Seed in current Level.                                                                                                               |
| Force Generate PCG | Removes all existing *PCG* actors in the current level and generates new ones. <br>**Note:** This is an *editor-only* function.                                                                                                          |
| Regenerate River | the current River is removed and a new River is regenerated on the current Landscape based on the River Setting. If you changed any weights on the Landscape that were affected by the River, those changes will be lost.                                                                                             |