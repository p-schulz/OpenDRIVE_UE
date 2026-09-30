using UnrealBuildTool;

public class OpenDriveEditor : ModuleRules
{
	public OpenDriveEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"OpenDrive"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ApplicationCore",
			"Slate",
			"SlateCore",
			"InputCore",
			"UnrealEd",
			"EditorFramework",
			"PropertyEditor",
			"DesktopPlatform",
			"AssetTools",
			"AssetRegistry",
			"AssetDefinition",
			"WorkspaceMenuStructure"
		});
	}
}
