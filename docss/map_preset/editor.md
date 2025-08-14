---
layout: default
title: MapPreset Editor
parent: MapPreset
nav_order: 6
---


# MapPreset Editor

이 섹션에서는 MapPreset 데이터 에셋의 커스텀 에디터에 대해 다룹니다.

MapPreset에셋은 Content Drawer에서 우클릭 > OCG > MapPreset 선택하여 생성할 수도 있습니다.

![Create New Map Preset]({{ site.baseurl }}/assets/images/map_preset/map_preset_editor/create_new_map_preset.png)


생성한 MapPreset 에셋을 더블 클릭하면 커스텀 에디터 창이 열리고, MapPreset을 편집할 수 있습니다.

![mapPresetEditor]({{ site.baseurl }}/assets/images/map_preset/map_preset_editor/mapPresetEditor.png)

- Preview Maps 버튼을 클릭하면, HeightMap File Path에 있는 .png 기반으로 생성될 Landscape를 초록색 선으로 미리 보여주고 프로젝트 하위에 Contetns/Maps 폴더에 HeightMap을 비롯한 다른 맵들을 png 파일로 저장합니다. 만약에 HeightMap File Path가 없을 경우, 현재 Seed 값 기반으로 생성될 Landscape를 초록색 선으로 미리 보여줍니다.
- Generate 버튼을 클릭하면 에디터의 뷰포트에 MapPreset 에셋을 기반으로 한 레벨이 생성됩니다.
- Export to Level 버튼을 클릭하면, 현재 뷰포트의 레벨을 실제 .umap 파일로 내보냅니다.
  - 내보낸 레벨은 레벨 스트리밍을 지원하지 않지만, 해당 레벨을 열고 상단의 Tools > Convert Level을 통해 레벨 스트리밍을 지원하는 레벨로 컨버트 할 수 있습니다.
  - 내보낸 레벨에서도 OCG Window를 열어 언제든지 MapPreset 에셋을 편집할 수 있습니다.
- Force Generate PCG 버튼을 클릭하면, PCG를 재생성 해줍니다.
- Regenerate River 버튼을 클릭하면, 현재 깔려 있는 River를 없애고, River Setting에 맞게 현재 Landscape에 다시 River를 재생성합니다.
  - 만약 River에 의해서 영향을 받았던, Landscape 지점에 Weight를 변경했다면 변경 사항이 사라집니다.

커스텀 에디터의 상단 Windows 드롭다운 메뉴를 통해 탭들을 켜고 끌 수 있습니다.

![MapPreset Editor Window]({{ site.baseurl }}/assets/images/map_preset/map_preset_editor/map_preset_editor_window.png)
