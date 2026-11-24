using UnrealBuildTool;

public class TwitchCppSample : ModuleRules
{
	public TwitchCppSample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Match the TwitchSDK plugin's settings. The sample passes std::function callbacks into
		// the plugin, and MSVC lays std::function out differently with and without RTTI, so both
		// modules must agree. Exceptions are needed because the core reports errors as std::exception.
		CppStandard = CppStandardVersion.Cpp20;
		bEnableExceptions = true;
		bUseRTTI = true;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"InputCore",
			"ApplicationCore", // clipboard
			"TwitchSDK",
		});
	}
}
