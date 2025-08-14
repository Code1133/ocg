---
title: OCG Basic Usage Tutorial
description: A step-by-step guide to using the One-Click Level Generator plugin.
layout: default
parent: Tutorials
nav_order: 1
---

# One-Click Level Generator Tutorial


## Basic Usage

### 1. Create a New Level

File > New Level(Ctrl+N)을 선택한 뒤, 새로운 빈 레벨을 생성합니다.

![New Level Select Image]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/new_level_select.png)

### 2. Open the One-Click Level Generator

Window > OCG Tools > OCG Window를 선택하여 OCG 창을 엽니다.

![Open OCG Window]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/open_ocg_window.png)

이후 OCG 창에서 Create Level Generator 버튼을 클릭하여 레벨 생성기를 월드에 추가합니다.

![Create Level Generator Button]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/create_level_generator.png)

OCG 창에서 `ㅤ+ㅤ` 버튼을 클릭후, 새로운 Map Preset 저장합니다.

![Add Map Preset Button]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/add_map_preset.png)

### 3. Configure the Map Preset

#### 3.1 Set World Settings

이 단계에서는 월드 설정을 구성합니다. 월드 설정은 레벨의 기본적인 환경과 속성을 정의합니다. OCG 창에서 World Settings 탭을 선택하고, 필요한 설정을 조정합니다.

![World Settings Tab]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/world_settings_tab.png)

<!-- TODO: @hans4809 @clairhardt98 여기 TODO부분 지우고 설명을 채워주시면 될 것 같습니다. 한국어로 적어도 딱히 상관 없을 듯 합니다. -->

> 각 설정에 대한 자세한 설명은 아래와 같습니다.
> <details markdown="1">
>   <summary>Landscape Settings</summary>
> 
> | Property Name                     | Description                                                                                                                                                                        |
> | :------------------------------- | :--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
> | Landscape Region Kilometer       | TODO                                                                                                                                                                               |
> | World Partition Grid Size        | TODO                                                                                                                                                                               |
> | World Partition Region Size      | TODO                                                                                                                                                                               |
> | Landscape Scale                  | Controls the size of the landscape by adjusting the Landscape Actor's scale.                                                                                                       |
> | Apply Scale To Noise             | When enabled, modifying the LandscapeScale will also alter the terrain's shape.                                                                                                    |
> | Biome Blend Radius               | Sets the blending radius in pixels between different biomes.                                                                                                                       |
> | Water Blend Radius               | Sets the blending radius in pixels between water and other biomes.                                                                                                                 |
> | Landscape Quads Per Section      | Specifies the number of quads per landscape section, which is the base unit for LOD transitions.                                                                                   |
> | Landscape Sections Per Component | Defines the number of sections per landscape component. This, combined with the section size, determines the overall size of a component, the base unit for rendering and culling. |
> | Landscape Component Count        | Sets the number of components in the X and Y axes, which defines the total size of the landscape.                                                                                  |
> | Map Resolution                   | The resolution of the landscape and its associated generation maps in the X and Y directions.                                                                                      |
> | Landscape Material               | The material to be applied to the landscape.                                                                                                                                       |
> 
> </details>
> 
> <details markdown="1">
>   <summary>Height</summary>
> 
> | Property Name | Description                                                                      |
> | :----------- | :------------------------------------------------------------------------------- |
> | Min Height   | The minimum possible height for the landscape.                                   |
> | Max Height   | The maximum possible height for the landscape.                                   |
> | Sea Level    | Determines the sea level, ranging from 0 (minimum height) to 1 (maximum height). |
> 
> </details>
> 
> <details markdown="1">
>   <summary>Temperature</summary>
> 
> | Property Name | Description                                         |
> | :----------- | :-------------------------------------------------- |
> | Min Temp     | The minimum possible temperature for the landscape. |
> | Max Temp     | The maximum possible temperature for the landscape. |
> 
> </details>
> 
> <details markdown="1">
>   <summary>Noise</summary>
> 
> | Property Name            | Description                                                           |
> | :---------------------- | :-------------------------------------------------------------------- |
> | Continent Noise Scale   | Controls the frequency of mountain generation.                        |
> | Terrain Noise Scale     | Controls the frequency of mountain generation.                        |
> | Temperature Noise Scale | Controls how frequently the temperature changes across the landscape. |
> 
> </details>
> 
> <details markdown="1">
>   <summary>River Settings</summary>
> 
> | Property Name                       | Description                                                                                                                                        |
> | :--------------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------- |
> | Generate River                     | Enables river generation. If checked, additional river settings will become available.                                                             |
> | River Count                        | The total number of rivers to generate.                                                                                                            |
> | River Source Elevation Ratio       | Sets the river's starting elevation. A value of 1.0 starts the river at the highest point, while 0.5 starts it at the mid-height of the landscape. |
> | River Spine Simplify Epsilon       | Controls the simplification intensity for the river's path. A higher value results in a straighter path.                                           |
> | River Width Base Value             | The base value for the river's width. The RiverWidthCurve is normalized and multiplied by this value to determine the final width.                 |
> | River Depth Base Value             | The base value for the river's depth. The RiverDepthCurve is normalized and multiplied by this value to determine the final depth.                 |
> | River Velocity Base Value          | The base value for the river's velocity. The RiverVelocityCurve is normalized and multiplied by this value to determine the final velocity.        |
> | River Width Min                    | The minimum width of the river. This value is added to the calculated width.                                                                       |
> | River Depth Min                    | The minimum depth of the river. This value is added to the calculated depth.                                                                       |
> | River Velocity Min                 | The minimum velocity of the river. This value is added to the calculated velocity.                                                                 |
> | River Width Curve                  | A curve that defines the river's width along its length. The X-axis is the distance from the source, and the Y-axis is the width.                  |
> | River Depth Curve                  | A curve that defines the river's depth along its length. The X-axis is the distance from the source, and the Y-axis is the depth.                  |
> | River Velocity Curve               | A curve that defines the river's velocity along its length. The X-axis is the distance from the source, and the Y-axis is the velocity.            |
> | River Water Material               | 강의 물을 표현하는데 사용되는 머티리얼입니다. WaterBodyRiver액터에 적용됩니다.|
> | River Water Static Mesh Material   | 강의 물을 표현하는데 사용되는 스태틱 메시의 머티리얼입니다. WaterBodyRiver액터에 적용됩니다.|
> | River To Lake Transition Material  | 강이 WaterBodyLake 액터와 오버랩될 때 적용되는 머티리얼입니다. WaterBodyRiver액터에 적용됩니다. |
> | River To Ocean Transition Material | 강이 WaterBodyOcean 액터와 오버랩될 때 적용되는 머티리얼입니다. WaterBodyRiver액터에 적용됩니다.|
> 
> </details>

<br>

#### 3.2 Set PCG Settings

이 단계에서는 PCG(Procedural Content Generation) 설정을 구성합니다. PCG 설정은 레벨의 생성 방식과 관련된 다양한 옵션을 제공합니다. OCG 창에서 PCG Settings 탭을 선택하고, 필요한 설정을 조정합니다.

##### Biome Settings

{: .warning }
> Landscape Material의 Landscape Layer Blend의 **Layer 순서와 Biome의 순서가 일치해야**, PCG가 **올바른 영역에 Mesh를 Spawn**할 수 있게 됩니다.
> 예를 들어, Landscape Material의 Layer 순서가 `Grass`, `Forest`, `Mountain`이라면, Biome의 순서도 **동일하게** `Grass`, `Forest`, `Mountain`이어야 합니다.

![PCG Settings Tab]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/biome_settings.png)

<details markdown="1">
  <summary>Biome Settings</summary>

| Property Name  | Description                                                                                                                     |
| :------------- | :------------------------------------------------------------------------------------------------------------------------------ |
| Biome Name     | The name of the biome. This is used to identify the biome in the system.                                                        |
| Temperature    | The temperature of the biome, which can affect the types of vegetation and terrain features generated.                          |
| Humidity       | The humidity level of the biome, influencing water features and vegetation density.                                             |
| Weight         | The weight of the biome in the overall generation process. Higher weights increase the likelihood of this biome being selected. |
| Mountain Ratio | The ratio of mountains in the biome, affecting the terrain's elevation and features.                                            |

</details>

##### Hierarchy Data Settings

{: .warning }
> Hierarchy Data Settings의 Index는 **설정된 순서대로 적용됩니다.**  
> 상위 Index에서 이미 포인트가 생성된 위치는, **다음 순서의 Index가 포인트를 생성할 수 있는 영역에서 제외됩니다.** (같은 바이옴은 제외)

![Hierarchy Data Settings]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/hierarchy_data_settings.png)

<details markdown="1">
  <summary>Basic Settings</summary>

| Property Name                 | Description                                                                                                                                                                                             |
| :---------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Biome Name                    | Biome의 이름입니다. 이 이름은 시스템에서 Biome을 식별하는 데 사용됩니다.                                                                                                                                |
| Selected Landscape Layer Name | 선택된 Landscape Layer의 이름입니다. 이 Layer에 Biome이 적용됩니다. <br> 만약 `None`일 경우 Biome Name이 잘못되었거나, Landscape Material을 설정하지 않은 경우입니다.                                   |
| Seed                          | 랜덤 시드를 설정합니다. <br> 이 값은 Biome의 Mesh가 Spawn될 때, 랜덤성을 결정합니다. <br> 동일한 Seed를 사용하면, 동일한 결과를 얻을 수 있습니다.                                                       |
| Blendig Ratio                 | Biome의 블렌딩 비율입니다. 이 값은 Biome과 Biome 사이 경계를 얼마나 부드럽게 할지를 결정합니다. <br> 0.0은 경계가 뚜렷하고, 1.0은 완전히 블렌딩됩니다.                                                  |
| Points Per Square Meter       | Biome의 Mesh가 Spawn되는 영역의 크기를 결정합니다. <br> 이 값은 Biome의 Mesh가 얼마나 세밀하게 Spawn될지를 결정합니다. <br> 예를 들어, 10이면 1제곱미터당 10개의 포인트가 생성됩니다.                   |
| Looseness                     | Biome의 Mesh가 Spawn되는 영역의 느슨함을 결정합니다.                                                                                                                                                    |
| Point Extents                 | Mesh가 Spawn될 영역의 크기를 결정합니다. <br> 이 값은 Point의 개수 계산에 영향을 미치며, Mesh가 Spawn될 영역의 크기를 결정합니다. <br> 예를 들어, 100이면 100x100 제곱미터의 영역에 Mesh가 Spawn됩니다. |
| Point Steepness               | Mesh가 Spawn될 때, 다른 Point와 얼만큼 떨어질 지 결정합니다. <br> 이 값이 높을수록 Mesh가 더 멀리 떨어져 Spawn됩니다.                                                                                   |
| Slope Limits                  | Mesh가 Spawn될 수 있는 경사의 범위를 결정합니다. <br> 예를 들어, Min Angle이 0°이고 Max Angle이 45°이면, 0°에서 45° 사이의 경사에서만 Mesh가 Spawn됩니다.                                               |
| Transform Point               | Point의 Transform을 결정합니다. <br> 이 값은 Mesh가 Spawn될 때, Mesh의 위치, 회전, 크기를 결정합니다.                                                                                                   |
| Pruning Overlapped Points     | Overlapped Points를 제거할지 여부를 결정합니다.                                                                                                                                                         |
| Meshes                        | Spawn할 Mesh를 선택합니다. <br> 이 값은 Biome의 Mesh가 Spawn될 때, 사용할 Mesh를 결정합니다. <br> 여러 개의 Mesh를 선택할 수 있으며, 랜덤하게 선택됩니다.                                               |
| Point Debug Color             | Point의 디버그 색상을 설정합니다. <br> 이 값은 Point가 Spawn될 때, 디버그 색상을 결정합니다. <br> 이 값을 사용하면, Point의 위치를 시각적으로 확인할 수 있습니다.                                       |

</details>

<details markdown="1">
  <summary>Optimization Settings</summary>

| Property Name                  | Description                                                                                                                                                                                                                                              |
| :----------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| WPO Disable Distance           | WPO(World Position Offset)를 비활성화할 거리를 결정합니다. <br> 이 값은 Mesh가 Spawn될 때, WPO가 적용되지 않는 거리를 결정합니다. <br> 예를 들어, 1000이면 1000미터 이상 떨어진 곳에서는 WPO가 적용되지 않습니다.                                        |
| Start Cull Distance            | Start Cull Distance를 설정합니다.                                                                                                                                                                                                                        |
| End Cull Distance              | End Cull Distance를 설정합니다. <br> 이 값은 Mesh가 Spawn된 후, 얼마나 멀리 떨어진 곳에서 Cull되는지를 결정합니다. <br> 예를 들어, Start Cull Distance가 1000이고 End Cull Distance가 2000이면, 1000미터 이상 떨어진 곳에서는 Mesh가 보이지 않게 됩니다. |
| Affect Distance Field Lighting | Distance Field Lighting에 영향을 줄지 여부를 결정합니다. <br> 이 값을 활성화하면, Mesh가 Distance Field Lighting에 영향을 미치게 됩니다. <br> 이 값은 성능에 영향을 줄 수 있으므로, 필요할 때만 활성화하는 것이 좋습니다.                                |
| Execute on GPU                 | GPU에서 실행할지 여부를 결정합니다. <br> 이 값을 활성화하면, Mesh가 GPU에서 생성됩니다.                                                                                                                                                                  |

</details>

<br>

#### 3.3 Set River Settings

<!-- TODO: 강 만드는 방법 -->
