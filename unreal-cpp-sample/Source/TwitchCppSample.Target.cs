using UnrealBuildTool;

public class TwitchCppSampleTarget : TargetRules
{
	public TwitchCppSampleTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TwitchCppSample");
	}
}
