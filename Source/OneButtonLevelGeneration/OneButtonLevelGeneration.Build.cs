// Copyright (c) 2025 Code1133. All rights reserved.

using UnrealBuildTool;

public class OneButtonLevelGeneration : ModuleRules
{
	public OneButtonLevelGeneration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// 공개 헤더에서 타입을 직접 노출하는 모듈
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"PCG",
			"PCGGeometryScriptInterop",
			"PCGWaterInterop",
		});

		// 내부에서 필요한 구현 전용 모듈
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"Foliage",
			"Landscape",
			"Projects",
			"Water",
		});

        // 에디터 전용 모듈
		if (Target.Type == TargetType.Editor)
		{
            PublicDependencyModuleNames.AddRange(new string[]
            {
                "EditorSubsystem",
            });

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"AssetRegistry",
				"AssetTools",
				"AutomationController",
				"ContentBrowser",
				"ContentBrowserData",
				"EditorStyle",
				"FunctionalTesting",
				"InputCore",
				"LandscapeEditor",
				"LevelEditor",
				"MaterialEditor",
				"PCGEditor",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				"VirtualTexturingEditor",
				"WaterEditor",
				"WorkspaceMenuStructure",
			});
		}
	}
}
