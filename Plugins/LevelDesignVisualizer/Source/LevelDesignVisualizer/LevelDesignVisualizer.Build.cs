// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LevelDesignVisualizer : ModuleRules
{
	public LevelDesignVisualizer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",                  // GEditor, FEditorDelegates
			"EditorSubsystem",           // UEditorSubsystem
			"AssetRegistry",             // Data Asset 자동 검색
			"EditorScriptingUtilities",  // UEditorActorSubsystem
			"Slate",
			"SlateCore",
		});
	}
}
