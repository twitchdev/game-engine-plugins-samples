// Some copyright should be here...
using System;
using System.IO;
using UnrealBuildTool;

public class TwitchSDK : ModuleRules
{
	public TwitchSDK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// find core library
		string coreLib = ModuleDirectory;
		while (true)
		{
			if (File.Exists(Path.Combine(coreLib, "core", "include", "twitchsdk.hpp")))
			{
				coreLib = Path.Combine(coreLib, "core");
				break;
			}

			var parent = Directory.GetParent(coreLib);
			if (parent == null)
				throw new Exception("TwitchSDK core library not found!");
			coreLib = parent.FullName;
		}

		Console.WriteLine("Using core library at " + coreLib);

		// Single source of truth for the version number: core/VERSION (see set-version.ps1).
		// Exposed publicly so game modules can use TWITCHSDK_VERSION as well.
		var versionFile = Path.Combine(coreLib, "VERSION");
		if (!File.Exists(versionFile))
			throw new Exception("TwitchSDK version file not found at " + versionFile);
		string version = File.ReadAllText(versionFile).Trim();
		if (!System.Text.RegularExpressions.Regex.IsMatch(version, @"^\d+\.\d+(\.\d+)?$"))
			throw new Exception("Invalid TwitchSDK version '" + version + "' in " + versionFile);
		PublicDefinitions.Add("TWITCHSDK_VERSION=\"" + version + "\"");

		// The .uplugin can't read core/VERSION, so set-version.ps1 stamps it. Warn if it's out of sync.
		var upluginFile = string.IsNullOrEmpty(PluginDirectory) ? null : Path.Combine(PluginDirectory, "TwitchSDK.uplugin");
		if (upluginFile != null && File.Exists(upluginFile))
		{
			var match = System.Text.RegularExpressions.Regex.Match(File.ReadAllText(upluginFile), "\"VersionName\"\\s*:\\s*\"([^\"]*)\"");
			if (match.Success && match.Groups[1].Value != version)
				Console.WriteLine("Warning: TwitchSDK.uplugin VersionName (" + match.Groups[1].Value + ") does not match core/VERSION (" + version + "). Run set-version.ps1 to sync them.");
		}

		var coreInclude = Path.Combine(coreLib, "include");
		var coreSrc = Path.Combine(coreLib, "src");

		CppStandard = CppStandardVersion.Cpp20;
		bEnableExceptions = true;
		bUseRTTI = true;

		PublicIncludePaths.AddRange(
			new string[] {
				coreInclude
			}
			);

		PrivateIncludePaths.AddRange(
			new string[] {
				coreSrc
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...
				"HTTP",
				"WebSockets",
				"Json",
				"DeveloperSettings",
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
