---
layout: default
title: Landscape Master Material Settings Guide
parent: World Settings Properties
nav_order: 5
---

# Landscape Master Material Settings Guide

This document provides a comprehensive guide to the OCG Landscape Master Material, detailing its structure, parameters, and how to effectively utilize it in your projects.

## 1. Layers
![BiomeToLayer]({{ site.baseurl }}/assets/images/map_preset/master_material/BiomeToLayer.png)

{: .warning }
> &emsp;The **order of layers** in the Landscape Material's Layer Blend **must match the order of Biomes** in the MapPreset.<br>
> &emsp;For example, if the Landscape Material's layer order is `Grass`, `Forest`, `Mountain`, the Biome order must also be **identical**: `Grass`, `Forest`, `Mountain`.<br>
> &emsp;By default, **Layer0** is the landscape area where **Water Body Actors from the Water Plugin are located**.<br>

- There are 8 layers provided by default. Each layer constitutes a single Biome Material.
  - You can configure the materials for Layers 1 through 7 to correspond with the order of the Biomes.

## 2. Material Instance Parameters
![MaterialInstnaceParamsBoxing]({{ site.baseurl }}/assets/images/map_preset/master_material/MaterialInstnaceParamsBoxing.png)
- In the Material Instance, you can view the parameters for each layer.
- Each layer uses the same logic from the Material Layers.

<details markdown="1">
  <summary><b style="font-size:1.25em;">All Layer Parameters</b></summary>

![AllLayerParams]({{ site.baseurl }}/assets/images/map_preset/master_material/AllLayerParams.png)

| Parameter Name            | Description                                                                                                                 |
| :------------------------ | :-------------------------------------------------------------------------------------------------------------------------- |
| Random Tiling Scale       | Larger values result in broader, smoother pattern changes. Smaller values create finer, more detailed tiling patterns.      |
| Random Tiling Levels      | Higher values add more complex, multi-layered variation, reducing texture repetition and creating a natural look.           |
| RVT Base Exponent         | Exponent value to control contrast (gamma) in RVT Mip-based fade. Higher values make the edge sharper.                      |
| RVT Distance Scale        | Scales the distance for RVT Mip-based fade. Larger values widen the fade region.                                            |
| Blend Distance Transition | The distance over which the fade transitions from visible to invisible. Larger values create a smoother fade.               |
| Blend Distance Start      | The world-space distance at which the fade begins. The transition starts from this distance.                                |
| Use ORD                   | If you enable this option, you can use the ORD texture; otherwise, you’ll use the Displacement texture and the ORM texture. |

{: .warning }
> &emsp;Currently, only **ORM & Displacement** or **ORD** texture formats are supported.<br>
> &emsp;If you have separate Ambient Occlusion, Roughness, Metallic, and Displacement textures, you must combine them into a single **ORM & Displacement Texture** for use.

- These parameters apply to all layers.

</details>
  
<details markdown="1">
  <summary><b style="font-size:1.25em;">Layer General Parameters</b></summary>
![BaseGeneralParams]({{ site.baseurl }}/assets/images/map_preset/master_material/BaseGeneralParams.png)

| Parameter Name      | Description                                                                                                                                     |
| :------------------ | :---------------------------------------------------------------------------------------------------------------------------------------------- |
| Add Color Variation | If you enable this option, you can Add ColorVariation in this Layer.                                                                            |
| Near Tiling Size    | The smaller the value, the denser the texture iteration, and the larger the area per tile, creating a near-field tiling that looks large.       |
| Far Tiling Size     | A small value maintains a detailed repetition pattern from a distance, and a large value emphasizes perspective with a rough large tile effect. |

<details markdown="1">
  <summary>Color Variation Settings</summary>

| Parameter Name            | Description                                                                                                                                                                                                      |
| :------------------------ | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Color Variation           | The larger the value, the larger the texture, resulting in a larger and wider pattern of color changes. The smaller the value, the more the texture repeats, creating a color change of smaller, finer patterns. |
| Color Variation Contrast  | The higher the value, the sharper and sharper the boundaries of color change, and the lower the value, the more smooth and gradual the boundaries are expressed.                                                 |
| Color Variation Intensity | A value of 0 does not show any effect at all, and the closer you get to 1, the stronger the color change is overwritten over the original color.                                                                 |

</details>
- These are the parameters that apply generally to the layer corresponding to the number in `(Number_BaseGeneral)`.
</details>

<details markdown="1">
  <summary><b style="font-size:1.25em;">Layer BaseGround Parameters</b></summary>


![BaseGroundBaseSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/BaseGroundBaseSetting.png)

{: .warning }
> &emsp;To reveal the ORD Texture parameter, you must check the **Use ORD** option in the **All Layer Parameters**.<br>
> ![GroundORD]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundORD.png)<br>
> &emsp;To reveal the Displacement Texture parameter, you must check the **(Number)_AddGroundDisplacement** option.<br>
> ![GroundDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundDisplacement.png)

| Parameter Name                   | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| :------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Ground Color                     | Base Color Of Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Ground Color Tint                | Change the overall color by applying a specific color tone to the existing color of the texture                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Ground Normal                    | Normal Of Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Ground Normal Intensity          | When the value is 1.0, the normal map retains its original strength.<br><br>When the value is between 0.0 and 1.0, the X and Y values of the normal map decrease, making the surface unevenness appear smooth and flat. For example, setting the value to 0.5 halves the intensity.<br><br>When the value is 0.0: Normal map effect disappears completely, making it look like a plane.<br><br>When the value is greater than 1.0, the X and Y values of the normal map are amplified, resulting in a deeper and stronger representation of the surface details. |
| Ground ORM                       | AmbientOcclusion / Roughness / Metalic Texture of Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Add Ground Far Tiling Variation  | Apply Far Tiling Variation In Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Add Ground Near Tiling Variation | Apply Near Tiling Variation In Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| Ground Far Tiling Factor         | Uniform scale factor applied to FarUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Ground Near Tiling Factor        | Uniform scale factor applied to NearUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Ground UV Aspect Ratio           | Aspect-ratio correction value applied to the final UVs so that textures maintain correct proportions.                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| Add Ground Displacement          | Apply Displacement Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Ground AO Intensity              | Controls the intensity of ambient occlusion on the ground surface. A value of 0 disables AO entirely, while higher values increase shadowing in crevices to enhance perceived depth.                                                                                                                                                                                                                                                                                                                                                                             |
| Ground Specular(Wetness)         | Adjusts specular reflectivity to simulate surface wetness. Lower values yield a dry, matte look; higher values produce glossy, reflective highlights as if the ground were wet.                                                                                                                                                                                                                                                                                                                                                                                  |
| Add Puddles                      | Add Puddle Effects in Ground                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| Ground Add Texture Variation     | If true, enables the texture-based variation blend between Ground and MiddelHigh                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |

<details markdown="1">
  <summary>Ground Displacement Setting</summary>

  ![GroundDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundDisplacement.png)
  ![GroundDisplacementSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundDisplacementSetting.png)

  | Parameter Name             | Description                                                                                                                                                                        |
  | :------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
  | Ground Displacement        | Displacement Texture Of Ground                                                                                                                                                     |
  | Ground Displacement Amount | Scales the magnitude of ground displacement. Larger values push vertices farther along their normals for pronounced surface relief; smaller values yield subtler height variation. |

</details>

<details markdown="1">
  <summary>Puddle Setting</summary>

  ![GroundPuddleSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundPuddleSetting.png)
  
  | Parameter Name   | Description                                                                                                                                                                                       |
  | :--------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
  | Puddle Size      | The larger the value, the larger the size of the puddle.                                                                                                                                          |
  | Puddle Constrain | The higher the value, the clearer and narrower the boundary of the puddle.                                                                                                                        |
  | Puddle Depth     | The higher the value, the stronger and more opaque the puddle effect, and the closer the value approaches zero, the weaker the effect, the more transparent it becomes and disappears completely. |
  | Puddle Clarity   | Sets the darkness of the area covered by the waterhole.                                                                                                                                           |

</details>

<details markdown="1">
  <summary>Texture Variation Setting</summary>

  ![GroundTextureVariationSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundTextureVariationSetting.png)

  {: .warning }
  > &emsp;This affects the area where BaseGround is applied by adding variation between the **BaseGround and Middle High of the corresponding layer**.<br>

  | Parameter Name                       | Description                                                                                             |
  | :----------------------------------- | :------------------------------------------------------------------------------------------------------ |
  | Ground Texture Variation Tiling Size | Controls the tiling scale of the variation texture. Higher values make the noise pattern appear larger. |
  | Ground Texture Variation Contrast    | Adjusts the contrast of the variation texture. <br>Values >1 amplify differences; <1 soften them.       |
  | Ground Texture Variation Intensity   | Sets how strongly the variation texture influences the final blend (0 = no effect, 1 = full effect).    |

</details>

<details markdown="1">
  <summary>Slope Setting</summary>

  {: .warning }
  > &emsp;This parameter is only visible when **Add Cliff is enabled for the corresponding layer**.<br>

![GroundSlopeSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/GroundSlopeSetting.png)

| Parameter Name         | Description                                                                                                          |
| :--------------------- | :------------------------------------------------------------------------------------------------------------------- |
| Ground Slope Constrain | The smaller the value, the more this layer is applied. (Apply in the order of BaseGround -> Middledle -> MiddleHigh) |

</details>

</details>

<details markdown="1">
  <summary><b style="font-size:1.25em;">Layer Cliff Parameters</b></summary>
  
![CliffParams]({{ site.baseurl }}/assets/images/map_preset/master_material/CliffParams.png)

  - These parameters are visible when Add Cliff is enabled.

  {: .warning }
  > &emsp;After **enabling Add Cliff**, check **Ground Slope Constrain** to adjust the slope value between the Cliff and BaseGround.<br>
  > &emsp;You must check **Use ORD** in the All Layer Parameters to reveal the ORD Texture parameter.<br>
  > ![CliffORD]({{ site.baseurl }}/assets/images/map_preset/master_material/CliffORD.png)<br>
  >  &emsp;You must check **(Number)_AddCliffDisplacement** to reveal the Displacement Texture parameter.<br>
  > ![CliffDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/CliffDisplacement.png)

| Parameter Name           | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| :----------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Cliff Color              | Base Color Of Cliff                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| Cliff Color Tint         | Change the overall color by applying a specific color tone to the existing color of the texture                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Cliff Normal             | Normal Of Cliff                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Cliff Normal Intensity   | When the value is 1.0, the normal map retains its original strength.<br><br>When the value is between 0.0 and 1.0, the X and Y values of the normal map decrease, making the surface unevenness appear smooth and flat. For example, setting the value to 0.5 halves the intensity.<br><br>When the value is 0.0: Normal map effect disappears completely, making it look like a plane.<br><br>When the value is greater than 1.0, the X and Y values of the normal map are amplified, resulting in a deeper and stronger representation of the surface details. |
| Cliff ORM                | AmbientOcclusion / Roughness / Metalic Texture of Cliff                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| Cliff Use Triplanar      | Apply Triplanar to Cliff                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Cliff Far Tiling Factor  | Uniform scale factor applied to FarUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Cliff Near Tiling Factor | Uniform scale factor applied to NearUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Cliff UV Aspect Ratio    | Aspect-ratio correction value applied to the final UVs so that textures maintain correct proportions.                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| Add Cliff Displacement   | Apply Displacement to Cliff                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| Cliff AO Intensity       | Controls ambient occlusion strength on cliff faces. A value of 0 removes AO shading, and increasing values darken crevices to accentuate rugged details.                                                                                                                                                                                                                                                                                                                                                                                                         |


<details markdown="1">
  <summary>Triplanar Setting</summary>

![TriplanarSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/TriplanarSetting.png)

| Parameter Name              | Description                |
| :-------------------------- | :------------------------- |
| Cliff Triplanar Tiling Size | Size of Applied Triplanar. |

  </details>

<details markdown="1">
  <summary>Displacement Setting</summary>

![CliffDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/CliffDisplacement.png)
![CliffDisplacementSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/CliffDisplacementSetting.png)

| Parameter Name            | Description                                                                                                                                      |
| :------------------------ | :----------------------------------------------------------------------------------------------------------------------------------------------- |
| Cliff Displacement        | Displacement Texture of Cliff                                                                                                                    |
| Cliff Displacement Amount | Determines the scale of cliff displacement. Higher values exaggerate rock protrusions and crevices; lower values smooth out the overall surface. |

  </details>

</details>

<details markdown="1">
  <summary><b style="font-size:1.25em;">Layer High Parameters</b></summary>

![HighParams]({{ site.baseurl }}/assets/images/map_preset/master_material/HighParams.png)

  {: .warning }
  > &emsp;You must check **Use ORD** in the All Layer Parameters to reveal the ORD Texture parameter.<br>
  > ![HighORD]({{ site.baseurl }}/assets/images/map_preset/master_material/HighORD.png)<br>
  >  &emsp;You must check **(Number)_AddHighDisplacement** to reveal the Displacement Texture parameter.<br>
  > ![HighDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/HighDisplacement.png)

| Parameter Name          | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| :---------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| High Color              | Base Color Of High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| High Color Tint         | Change the overall color by applying a specific color tone to the existing color of the texture                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| High Normal             | Normal Of High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| High Normal Intensity   | When the value is 1.0, the normal map retains its original strength.<br><br>When the value is between 0.0 and 1.0, the X and Y values of the normal map decrease, making the surface unevenness appear smooth and flat. For example, setting the value to 0.5 halves the intensity.<br><br>When the value is 0.0: Normal map effect disappears completely, making it look like a plane.<br><br>When the value is greater than 1.0, the X and Y values of the normal map are amplified, resulting in a deeper and stronger representation of the surface details. |
| High ORM                | AmbientOcclusion / Roughness / Metalic Texture of High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| High Near Tiling Factor | Uniform scale factor applied to NearUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| High Far Tiling Factor  | Uniform scale factor applied to FarUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| High UV Aspect Ratio    | Uniform scale factor applied to FarUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| High Slope Contrast     | The value that controls the strength of the mask.<br><br>The larger the value, the more dramatic the transition of the mask near the gradient threshold, and the smaller the value, the smoother it is.                                                                                                                                                                                                                                                                                                                                                          |
| High Start Height       | Reference height (Z, world unit) at which the Height Mask starts to be activated at 0. The mask applies when the absolute Z value of the mesh is greater than or equal to this height.                                                                                                                                                                                                                                                                                                                                                                           |
| High Transition         | The smaller the value, the more pronounced and sharp boundaries are created.<br><br>The larger the value, the more smooth and gradual the gradation is created.                                                                                                                                                                                                                                                                                                                                                                                                  |
| High Edge Noise Tiling  | Smaller values repeat dense small noise tiles, and larger values produce rougher and larger noise patterns.                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| High Edge Contrast      | An index value that adjusts the strength of the sampled noise values.<br><br>Larger values result in stark differences between brighter and darker regions, resulting in distinct noise effects, and smaller values result in smoother transitions.                                                                                                                                                                                                                                                                                                              |
| High Edge Noise Amount  | A scalar that controls the intensity of the final edge noise.<br><br>Multiply the saturated noise result by this value in the multiplexing step to adjust the overall noise application.                                                                                                                                                                                                                                                                                                                                                                         |
| High Projection Angle   | Specifies the maximum slope angle (in degrees) at which the high layer’s projection mapping is applied. Lower values restrict projection to very steep surfaces, while higher values extend it to gentler slopes.                                                                                                                                                                                                                                                                                                                                                |
| High Slope Constrain    | Minimum angle of inclination at which the mask starts.<br><br>Mask is active from the point where SlopeAngle is above this value.                                                                                                                                                                                                                                                                                                                                                                                                                                |
| Add High Displacement   | Apply Displacement High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| High AO Intensity       | Adjusts ambient occlusion on high-elevation surfaces. A value of 0 disables AO, while higher values intensify shading in crevices to highlight fine details.                                                                                                                                                                                                                                                                                                                                                                                                     |


<details markdown="1">
  <summary>Displacement Setting</summary>

![HighDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/HighDisplacement.png)
![HighDisplacementSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/HighDisplacementSetting.png)

| Parameter Name           | Description                                                                                                                                                         |
| :----------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| High Displacement        | Displacement Texture of High                                                                                                                                        |
| High Displacement Amount | Controls displacement strength for the high layer. Larger values create more pronounced surface offsets and sharp detail; smaller values keep the terrain smoother. |

  </details>

</details>

<details markdown="1">
  <summary><b style="font-size:1.25em;">Layer Middle Parameters</b></summary>

![MiddleParams]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleParams.png)

  {: .warning }
  > &emsp;You must check **Use ORD** in the All Layer Parameters to reveal the ORD Texture parameter.<br>
  > ![MiddleORD]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleORD.png)<br>
  >  &emsp;You must check **(Number)_AddMiddleDisplacement** to reveal the Displacement Texture parameter.<br>
  > ![MiddleDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleDisplacement.png)

| Parameter Name            | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| :------------------------ | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Middle Color              | Base Color Of Middle                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Middle Color Tint         | Change the overall color by applying a specific color tone to the existing color of the texture                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Middle Normal             | Normal Of Middle                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| Middle Normal Intensity   | When the value is 1.0, the normal map retains its original strength.<br><br>When the value is between 0.0 and 1.0, the X and Y values of the normal map decrease, making the surface unevenness appear smooth and flat. For example, setting the value to 0.5 halves the intensity.<br><br>When the value is 0.0: Normal map effect disappears completely, making it look like a plane.<br><br>When the value is greater than 1.0, the X and Y values of the normal map are amplified, resulting in a deeper and stronger representation of the surface details. |
| Middle ORM                | AmbientOcclusion / Roughness / Metalic Texture of Middle                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Middle Far Tiling Factor  | Uniform scale factor applied to FarUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Middle Near Tiling Factor | Uniform scale factor applied to NearUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Middle UV Aspect Ratio    | Aspect-ratio correction value applied to the final UVs so that textures maintain correct proportions.                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| Middle Slope Constrain    | The smaller the value, the more this layer is applied. (Apply in the order of BaseGround -> Middledle -> MiddleHigh)                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Add Middle Displacement   | Apply Displacement Middle                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Middle AO Intensity       | Controls ambient occlusion on mid‑elevation surfaces. A value of 0 removes AO shading; increasing values deepen shadows in surface recesses to enhance texture.                                                                                                                                                                                                                                                                                                                                                                                                  |

<details markdown="1">
  <summary>Displacement Setting</summary>

![MiddleDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleDisplacement.png)
![MiddleDisplacementSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleDisplacementSetting.png)


| Parameter Name             | Description                                                                                                                                               |
| :------------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Middle Displacement        | Displacement Texture of Middle                                                                                                                            |
| Middle Displacement Amount | Scales the displacement effect on the middle layer. Higher values produce more pronounced bumps and ridges; lower values yield gentler surface variation. |

  </details>

</details>

<details markdown="1">
  <summary><b style="font-size:1.25em;">Layer Middle High Parameters</b></summary>

![MiddleHighParams]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleHighParams.png)

  {: .warning }
  > &emsp;You must check **Use ORD** in the All Layer Parameters to reveal the ORD Texture parameter.<br>
  > ![MiddleHighORD]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleHighORD.png)<br>
  >  &emsp;You must check **(Number)_AddMiddleHighDisplacement** to reveal the Displacement Texture parameter.<br>
  > ![MiddleHighDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleHighDisplacement.png)

| Parameter Name                 | Description                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| :----------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Middle High Color              | Base Color Of Middle High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Middle High Color Tint         | Change the overall color by applying a specific color tone to the existing color of the texture                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| Middle High Normal             | Normal Of Middle High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| Middle High Normal Intensity   | When the value is 1.0, the normal map retains its original strength.<br><br>When the value is between 0.0 and 1.0, the X and Y values of the normal map decrease, making the surface unevenness appear smooth and flat. For example, setting the value to 0.5 halves the intensity.<br><br>When the value is 0.0: Normal map effect disappears completely, making it look like a plane.<br><br>When the value is greater than 1.0, the X and Y values of the normal map are amplified, resulting in a deeper and stronger representation of the surface details. |
| Middle High ORM                | AmbientOcclusion / Roughness / Metalic Texture of Middle High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| Middle High Far Tiling Factor  | Uniform scale factor applied to FarUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| Middle High Near Tiling Factor | Uniform scale factor applied to NearUV before passing into the tiling-variation function.                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| Middle High UV Aspect Ratio    | Aspect-ratio correction value applied to the final UVs so that textures maintain correct proportions.                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| Middle High Slope Constrain    | The smaller the value, the more this layer is applied. (Apply in the order of BaseGround -> Middle Highdle -> Middle HighHigh)                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| Add Middle High Displacement   | Apply Displacement Middle High                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| Middle High AO Intensity       | Adjusts ambient occlusion for the middle‑high layer. A value of 0 disables AO completely, while higher values darken cracks and overhangs for richer depth cues.                                                                                                                                                                                                                                                                                                                                                                                                 |

<details markdown="1">
  <summary>Displacement Setting</summary>

![MiddleHighDisplacement]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleHighDisplacement.png)
![MiddleHighDisplacementSetting]({{ site.baseurl }}/assets/images/map_preset/master_material/MiddleHighDisplacementSetting.png)

| Parameter Name                  | Description                                                                                                                                                 |
| :------------------------------ | :---------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Middle High Displacement        | Displacement Texture of Middle High                                                                                                                         |
| Middle High Displacement Amount | Determines the magnitude of displacement on middle‑high geometry. Larger values accentuate relief and detail; smaller values maintain a flatter appearance. |

  </details>

</details>