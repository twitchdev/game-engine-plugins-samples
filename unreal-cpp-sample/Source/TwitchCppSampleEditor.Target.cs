using UnrealBuildTool;

public class TwitchCppSampleEditorTarget : TargetRules
{
	public TwitchCppSampleEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TwitchCppSample");
	}
}
