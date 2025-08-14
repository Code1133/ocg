---
layout: default
title: River Settings Properties
parent: MapPreset
nav_order: 3
---

# River Settings

This document explains how to generate rivers in a level using the MapPreset asset and describes the properties in the River Settings section.

## The river generation process via MapPreset is as follows:
1. A random location at a certain height above sea level is chosen as the starting point.
2. A Spline is generated that travels down to sea level and is then simplified.
3. A WaterBody River actor is created using this Spline.
4. The river's width, depth, and velocity are determined.
   1. The configured Curve data is normalized.
   2. The final river width, depth, and velocity are determined by multiplying the normalized Curve data value by the Base Value.

## Map Preset River Settings
![MapPreset River Settings]({{ site.baseurl }}/assets/images/map_preset/river_settings/river_settings.png)

## River Generation Parameters

| Property Name                 | Description                                                                                                                                                                                                       |
| :---------------------------- | :---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Regenerate River              | Removes all existing rivers in the current level and generates new ones.<br>**Note:** This function can only be called in the editor.                                                                             |
| River Seed                    | This is the seed for random river generation. Rivers with the same shape will be generated in the same location when using the identical seed.                                                                    |
| Generate River                | Determines whether to generate rivers in the current level.                                                                                                                                                       |
| River Count                   | The number of rivers to generate. A maximum of 10 rivers can be created.                                                                                                                                          |
| River Source Elevation Ratio  | A value that determines how high the river's starting point is from sea level.<br>You can specify a value between 0.5 and 1. The closer to 1, the higher up on the current landscape the river will be generated. |
| River Spline Simplify Epsilon | A value that determines how much to simplify the generated river's Spline.<br>A higher value results in a more simplified Spline.                                                                                 |

## Parameters for River Width, Depth, and Velocity

| Property Name | Description                                                                                                                                                                                                        |
| :------------ | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Base Value    | The base value for the river's width, depth, and velocity.<br>It is multiplied by the normalized Curve value to represent the actual river.                                                                        |
| Curve Data    | Determines how the width, depth, and velocity change as the river flows.<br>This is a float curve where the X-axis represents the river's progress, and the Y-axis represents the width, depth, or velocity value. |
| Materials     | The material properties to be set on the generated Water Body River actor.                                                                                                                                         |