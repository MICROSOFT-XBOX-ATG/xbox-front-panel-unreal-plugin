//*********************************************************
// Copyright (c) Microsoft. All rights reserved.
//*********************************************************
using UnrealBuildTool;

public class XboxFrontPanel : ModuleRules
{
	public XboxFrontPanel(ReadOnlyTargetRules Target) : base(Target)
	{
		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"ApplicationCore",
				"CoreUObject",
				"RenderCore",
				"Renderer",
				"Engine",
				"RHI",
				"Slate",
				"UMG",
				"InputCore"
			});

		if (Target.Platform == UnrealTargetPlatform.XboxOne)
		{
			PublicAdditionalLibraries.Add("XboxFrontPanel.lib");
		}
	}
};