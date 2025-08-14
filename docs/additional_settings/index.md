---
title: Additional Settings
layout: default
tags: [Additional Settings]
nav_order: 5
---

# Additional Settings

## Nanite Setting
- To enable Nanite tessellation, add the following to your project’s Config/DefaultEngine.ini:
```ini
[/Script/Engine.RendererSettings]
r.Nanite.AllowTessellation=1
r.Nanite.Tessellation=1
```
  
<br>![LandscapeNaniteSetting]({{ site.baseurl }}/assets/images/additional_settings/LandscapeNaniteSetting.png)
- If you want to apply Nanite Tesselation to Landscape, check Landscape Enable Nanite and build Data. 

## Runtime Virtual Texture Setting
![RVTSetting]({{ site.baseurl }}/assets/images/additional_settings/RVTSetting.png)
- In Editor, Edit -> Project Settings -> Engine - Rendering -> Virtual Textures
- Check Enable virtual support and UnCheck Enable virtual texture on texture import

![MF_RVTBlend]({{ site.baseurl }}/assets/images/additional_settings/MF_RVTBlend.png)
- In Material Editor where you want to apply the Color/Normal Blend using RVT (etc. PCG Mesh's Material)-> Right-Click -> Search MF_RVTBlend

![Connect_RVTBlend]({{ site.baseurl }}/assets/images/additional_settings/Connect_RVTBlend.png)
- Connect the final Base Color and Normal as inputs to RVT_Blend, and link the output to the Result Node.
