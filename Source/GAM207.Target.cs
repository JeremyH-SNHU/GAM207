using UnrealBuildTool;
using System.Collections.Generic;

public class GAM207Target : TargetRules
{
	public GAM207Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("GAM207");
	}
}
