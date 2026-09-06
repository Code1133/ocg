---
title: Quick Start
layout: default
tags: [quick start, getting started]
nav_order: 2
---

# One-Click Level Generator Quick Start
This section explains how to quickly create a level using the plugin's sample map preset asset and landscape material.

## Basic Usage

### 1. Create a New Level

Select File > New Level (Ctrl+N) to create a new empty level. OCG also supports open worlds.

![New Level Select Image]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/new_level_select.png)

### 2. Open the One-Click Level Generator

Click the **OCG** button on the level editor toolbar, or select **Window > OCG Tools > OCG**.

![Open OCG Window]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/open_ocg_window.png)

{: .info }
> Changed in 2.0.0. There is no longer a `LevelGenerator` actor to place in the level — pick a
> preset in the window and generate.

In the **Preset** field at the top of the window, assign the `MP_StylizedLandscape` map preset
located under Plugins/One-Click Level Generation/Samples.

![Set Sample Map Preset]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/set_sample_mappreset.png)

Check if the PCG graph and landscape material are properly assigned. If not, assign them as shown in the image below.

![Check Map Preset Graph and Material]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/check_map_preset.png)

Click **Generate All** to start generating the level.

![Click Generate Button]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/click_generate.png)

For UE 5.6 or later versions, a Layer Settings window may pop up. Ensure the Water layer is above the Layer layer, then click the Done button.

![Check Layer Order]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/check_layer_order.png)

Select Window > Environment Light Mixer from the top menu to add environment lighting-related actors to the level.

![Set Env Lights]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/set_env_light.png)

Your sample level is now generated! Refer to the MapPreset documentation to create your own custom levels.

![Set Env Lights]({{ site.baseurl }}/assets/images/tutorials/basic_tutorial/sample_level_generated.png)
