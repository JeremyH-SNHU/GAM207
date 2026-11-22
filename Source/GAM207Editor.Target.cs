using UnrealBuildTool;
using System.Collections.Generic;

public class GAM207EditorTarget : TargetRules
{
	public GAM207EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("GAM207");
	}
}
