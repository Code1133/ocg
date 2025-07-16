---
layout: default
title: PCG Properties 
parent: MapPreset
nav_order: 1
---

# PCG Properties

This section covers the *PCG* properties within the `MapPreset` asset.

![map_preset_pcg]({{ site.baseurl }}/assets/images/map_preset/pcg_properties/map_preset_pcg.png)

## PCG Properties

| Property Name  | Description                                                                                                                                                                                                                              |
| :------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Force Generate | Removes all existing *PCG* actors in the current level and generates new ones. <br>**Note:** This is an *editor-only* function.                                                                                                          |
| PCG Graph      | The *PCG graph* assigned to this `MapPreset` asset.                                                                                                                                                                                      |
| Auto Generate  | Determines whether the *PCG graph* should be generated automatically. <br>If *checked*, PCG will run automatically whenever a property is changed. <br>If *unchecked*, you must click the **Force Generate** button to generate the PCG. |
