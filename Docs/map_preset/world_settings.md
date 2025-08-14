---
layout: default
title: World Settings Properties
parent: MapPreset
has_children: true
nav_order: 2
---

# World Setting Properties

This page details the properties available in the World Settings section of the MapPreset asset. These settings are fundamental to configuring the procedural landscape generation process.

## 1. Basic World Settings

<details markdown="1">
<summary>Landscape Settings</summary>
  
  ![landscapeSettings]({{ site.baseurl }}/assets/images/map_preset/world_settings/landscapeSettings.png)
  
| Property Name | Description |
| :------------ | :---------- ||
| World Partition Grid Size        | The number of landscape components per axis for streaming proxies                                                                                                                                                                               |
| World Partition Region Size      | The number of landscape components per World Partition region, along each axis                                                                                                                                                                               |
| Landscape Size                   | Defines the horizontal size of the Landscape (in kilometers), it changes Landscape Size by Formular (XScale = Landscape Size * 1000 * 100 / Map Resolution.)                                                                                                       |
| Apply Scale To Noise             | If enabled, changing Landscape Scale regenerates the landscape's features. If disabled, it only stretches or shrinks the Landscape, preserving its shape.                                                                                                    |
| Debug Grid Spacing               | Defines the grid spacing of debug lines shown when Preview Maps button is clicked.                                                                                                    |
| Biome Blend Radius               | Defines the radius of the transition zone between adjacent biomes. This value controls the distance over which one biome's influence fades into the next, preventing abrupt changes. A value of 0 will disable blending, creating hard edges.                                                                                                                       |
| Water Blend Radius               | This property does same actions as Biome Blend Radius but only decides trasition between water biome and other biomes.                                                                                                                 |
| Landscape Quads Per Section      | The number of quads in a single landscape section. One section is the unit of LOD transition for landscape rendering.                                                                                   |
| Landscape Sections Per Component | The number of sections in a single landscape component. This along with the section size determines the size of each landscape component. A component is the base unit of rendering and culling. |
| Landscape Component Count        | Sets the number of components in the X and Y axes, which defines the total size of the landscape.                                                                                  |
| Map Resolution                   | The resolution of the landscape and its associated generation maps in the X and Y directions.                                                                                      |
| Landscape Material               | The material to be applied to the landscape.                 |           
| Heightmap File Path              | .png file Path, which apply to Landscape Height.                                                           

</details>

<details markdown="1">
<summary>Height & Temperature</summary>

![Height Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/height_settings.png)

![Temperature Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/temperature_settings.png)

| Property Name           | Description                                                                                                                                                               |
| :---------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Min Height / Max Height | Decides the minimum / maximum height of the landscape formed in unreal unit (1uu = 1cm). Changing this value decides the Z scale of the landscape.                        |
| Sea Level               | Defines the sea level as a normalized value (0.0 - 1.0) that interpolates between Min Height and Max Height. A value of 0 corresponds to Min Height, and 1 to Max Height. |
| Min Temp / Max Temp     | Defines the minimum / maximum temperature of your level. Unit is degree Celcius.                                                                                          |

</details>
 
<details markdown="1">
<summary>Noise</summary>

![Noise Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/noise_settings.png)

| Property Name           | Description                                                                                                                                                                                                                                                                                                                                            |
| :---------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Continent Noise Scale   | Controls the frequency of the noise that generates the main landmasses. Low values create wide mountain ranges, while high values result in bumpier, more rugged terrain with dense hills.                                                                                                                                                             |
| Terrain Noise Scale     | Governs the scale of the fractal noise that adds fine details to the terrain surface. This acts as the base frequency for the multi-octave noise generation. Lower values produce a smoother and more gently rolling landscape, while higher values introduce more high-frequency undulations, resulting in a more rugged and detailed ground surface. |
| Temperature Noise Scale | Controls the scale of the noise that generates the base temperature map. Low values result in a more uniform base temperature across the entire map. High values introduce significant variations, creating many distinct regions. This base temperature is then adjusted by elevation, becoming progressively colder at higher altitudes.             |

</details>

### Advanced Properties

<details markdown="1">
<summary>Height</summary>

![Advanced Height Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/advanced_height.png)

| Property Name                   | Description                                                                                                                                                                                                                                                                                                                                         |
| :------------------------------ | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Cur Min Height / Cur Max Height | This property is read-only property which shows the actual minimum height and maximum height of the landscape generated.                                                                                                                                                                                                                            |
| Smooth Height                   | If enabled height of the landscape is smoothed by Gaussian blur method. Enabling this generates smoother landscape. It  is recommended to always enable this option to avoid small stripes on the landscape.                                                                                                                                        |
| Gaussian Blur Radius            | Controls the intensity and radius of the Gaussian blur effect. This value defines how many neighboring pixels are averaged together to smooth the heightmap. Larger values produce a much softer and more heavily blurred appearance, effectively removing fine details and sharp edges. A smaller value results in a more subtle smoothing effect. |
| Smooth by Slope                 | This property is shown when Smooth Height is enabled. When enabled, this performs an additional smoothing pass specifically designed to flatten terrain areas that exceed the Max Slope Angle. Enabling this option provides a smoother landscape, but some of the landscape's detail may be lost.                                                     |
| Smoothing Iteration             | The number of times smooth by slope logic is applied. Larger iteration gives stronger smoothing effect but takes more time.                                                                                                                                                                                                                         |
| Smoothing Strength              | Controls the intensity of the slope-based smoothing effect. It determines how much a pixel's original height is blended towards its new, calculated smoothed height. A value of 1.0 applies the full correction, while a value of 0.0 applies no correction at all.                                                                                 |
| Smooth by Medium Height         | Applies a median filter, which is excellent for removing sharp, isolated noise like single-pixel spikes or artifacts. For each pixel, it samples the heights of its neighbors and uses the median value as the new height. Enabling this option provides a smoother landscape, but some of the landscape's detail may be lost.                         |
| Median Smooth Radius            | Defines the size of the area used to find the median height. A larger radius creates a stronger effect and can remove bigger noise clusters, but may also soften sharp details.                                                                                                                                                                     |
| Island                          | Controls whether the final landscape is shaped like an island or fills the entire map area. If disabled, the other Island Properties will have no effect.                                                                                                                                                                                           |
| Island Falloff Exponent         | Controls the steepness of the island's coastline falloff. Higher values create a sharper, more dramatic drop-off, resulting in steep cliffs. Lower values produce a gentler, more gradual slope, creating the appearance of beaches or soft shores.                                                                                                 |
| Island Shape Noise Scale        | Adjusts the frequency of the noise used to generate the island's overall coastline shape. Larger values introduce more frequent, smaller details, resulting in a more complex and jagged coastline. Smaller values create a smoother, more large-scale and simplified island shape.                                                                 |
| ModifyTerrainByBiome            | Enables a post-processing pass that modifies the terrain based on the underlying biome data. This allows each biome to have its own unique surface characteristics, such as smoothness, detail frequency, and detail height.                                                                                                                        |
| Plain Smooth Factor             | Controls how much smoothing is applied to areas defined as plains by the biome map. A value of 1.0 applies the full smoothing effect, creating very flat plains. A value of 0.0 applies no extra smoothing.                                                                                                                                         |
| Biome Noise Scale               | Sets the scale of the fine-grained noise used to add surface detail within each biome. This works similarly to TerrainNoiseScale but is modulated by the biome type. It controls the frequency of local bumps, rocks, and other small features.                                                                                                     |
| Biome Noise Amplitude           | Determines the intensity, or height, of the local biome noise. This controls how tall or deep the small surface details are within each biome.                                                                                                                                                                                                      |
| Biome Height Blend Radius       | Controls the blending distance for terrain modifications at the border between different biomes. A larger radius creates a wider, more gradual transition, preventing abrupt changes in terrain texture (e.g., from a bumpy to a smooth area). A value of 0 will create a sharp, distinct line.                                                     |

</details>

<details markdown="1">
<summary>Temperature & Humidity</summary>

![Advanced Temperature and Height Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/advanced_temp_humidity_settings.png)

| Property Name                     | Description                                                                                                                                                                                                                                                                                                                                                         |
| :-------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Temp Drop Per 1000 Units          | Specifies the rate at which temperature decreases with increasing altitude. This value represents the total temperature drop for every 1000 world units of elevation above sea level. Higher values will result in colder mountaintops and a more significant temperature difference between low and high ground.                                                   |
| Moisture Falloff Rate             | Determines how rapidly humidity decreases as the distance from a water source increases. This value acts as a decay factor in an exponential falloff function. Higher values cause a steep drop, resulting in narrow, humid coastal regions and very dry inland areas. Lower values create a much more gradual decline, allowing moisture to extend farther inland. |
| Temperature Influence On Humidity | Controls how strongly temperature reduces humidity. This simulates the effect where hot regions are typically drier. A value of 1.0 means that the hottest areas will have their humidity significantly reduced, effectively creating deserts. A value of 0.0 means temperature has no influence, and humidity is determined only by proximity to water.            |

</details>

<details markdown="1">
<summary>Noise</summary>

![Advanced Temperature and Height Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/advanced_noise_settings.png)

| Property Name         | Description                                                                                                                                                                                                                                                            |
| :-------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Standard Noise Offset | Sets the maximum random offset applied to each noise layer. This ensures that different noise passes (e.g., for continents, details, islands) are sampled from unique locations, preventing visual repetition and increasing overall randomness.                       |
| Redistribution Factor | Adjusts the contrast of the procedural masks that blend different features. Higher values create sharper transitions, resulting in flatter plains and more distinct mountain areas with less gradual falloff.                                                          |
| Octaves               | Sets the number of noise layers used in the fractal noise generation (FBM). More octaves add more layers of fine detail to the landscape, creating a more complex and realistic surface at the cost of performance.                                                    |
| Lacunarity            | In fractal noise generation, this controls the frequency multiplier between octaves. Higher values create finer, more tightly-packed details, contributing to a more rugged surface texture.                                                                           |
| Persistence           | In fractal noise generation, this controls the amplitude multiplier between octaves. Lower values reduce the influence of finer details, resulting in a smoother overall landscape. Higher values allow fine details to be more prominent, creating a rougher surface. |

</details>

<details markdown="1">
<summary>Erosion</summary>

![Advanced Temperature and Height Properties]({{ site.baseurl }}/assets/images/map_preset/world_settings/erosion_settings.png)

| Property Name            | Description                                                                                                                                                                                                                                                                                                      |
| :----------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Erosion                  | Enables or disables the hydraulic erosion simulation pass. When enabled, this pass simulates the effect of water droplets flowing over the terrain, carving channels and creating more realistic landforms.                                                                                                      |
| Num Erosion Iterations   | Sets the total number of water droplets simulated during the erosion process. This is the primary control for the overall quality and intensity of the erosion. More iterations produce a more detailed and heavily eroded landscape but significantly increase processing time.                                 |
| Erosion Radius           | Defines the radius (in pixels) of a droplet's "brush," which determines the area of effect when it erodes or deposits sediment. A larger radius creates wider, softer channels, while a smaller radius results in more incised, sharper gullies.                                                                 |
| Droplet Inertia          | Controls how much a droplet tends to continue in its current direction versus immediately following the steepest path. A value near 1.0 creates high inertia, resulting in longer, smoother, more powerful-looking riverbeds. A value near 0.0 makes the droplet's path highly sensitive to local slope changes. |
| Sediment Capacity Factor | A multiplier that affects how much sediment a droplet can carry, which is based on its speed, water volume, and the local slope. Higher values allow droplets to pick up more soil, leading to deeper and more dramatic erosion channels.                                                                        |
| Min Sediment Capacity    | A baseline capacity for sediment that a droplet always has, preventing erosion from stopping completely on nearly flat terrain.                                                                                                                                                                                  |
| Erode Speed              | Controls the rate at which a droplet removes soil from the terrain when it is below its sediment capacity. Higher values result in faster and deeper carving of the landscape.                                                                                                                                   |
| Deposit Speed            | Controls the rate at which a droplet deposits its carried sediment when it is over its capacity or moving uphill. This is responsible for creating features like alluvial fans and sediment banks.                                                                                                               |
| Evaporate Speed          | The rate at which a droplet loses water as it travels. This gradually reduces its speed and sediment capacity, causing it to deposit its load and creating more realistic flow patterns.                                                                                                                         |
| Gravity                  | A constant that accelerates the droplet based on the terrain's height difference, simulating gravitational force. This directly influences the droplet's speed calculation.                                                                                                                                      |
| Max Droplet Lifetime     | The maximum number of steps a single droplet can travel before it is removed from the simulation. This prevents infinite loops and effectively limits the maximum length of any single erosion path.                                                                                                             |
| Initial Water Volume     | The amount of water each simulated droplet starts with. Droplets with more water can generally carry more sediment and maintain their speed for longer.                                                                                                                                                          |
| Initial Speed            | The speed each droplet has at the moment it is placed on the landscape.                                                                                                                                                                                                                                          |

</details>