using UnrealBuildTool;
using System.Collections.Generic;

public class Owlbear3DTarget : TargetRules
{
    public Owlbear3DTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Owlbear3D");
    }
}

