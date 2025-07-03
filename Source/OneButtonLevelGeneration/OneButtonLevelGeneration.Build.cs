// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OneButtonLevelGeneration : ModuleRules
{
	public OneButtonLevelGeneration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"PCG",
				"PCGGeometryScriptInterop",
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetTools",
				"EditorStyle",
				"Engine",
				"Landscape",
				"LandscapeEditor",
				"PCGEditor",
				"PropertyEditor",
				"Slate",
				"SlateCore",
				"UnrealEd",
				"WorkspaceMenuStructure",
				"MaterialEditor",
				"InputCore",
				"DeveloperSettings", 
				"VirtualTexturingEditor",
				"Projects"
			}  
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
