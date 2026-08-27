using UnrealBuildTool;

public class Owlbear3D : ModuleRules
{
    public Owlbear3D(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Slate",
            "SlateCore"
        });
    }
}
