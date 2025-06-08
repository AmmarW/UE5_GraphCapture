// File: UE5_GraphCapture.Build.cs
using UnrealBuildTool;

public class UE5_GraphCapture : ModuleRules
{
    public UE5_GraphCapture(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivatePCHHeaderFile = "Public/UE5_GraphCapture.h";

        PublicIncludePaths.AddRange(new string[] {
            "UE5_GraphCapture/Public"
        });

        PrivateIncludePaths.AddRange(new string[] {
            "UE5_GraphCapture/Private"
        });

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new string[] {
            "Projects",
            "InputCore",
            "UnrealEd",
            "LevelEditor",
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "NiagaraEditor",
            "AssetRegistry",
            "Kismet",
            "GraphEditor",
            "Json",             // for JSON DOM & writer :contentReference[oaicite:0]{index=0}
            "JsonUtilities",    // for FJsonObjectConverter :contentReference[oaicite:1]{index=1}
            "ApplicationCore",
            "ImageWriteQueue",
            "MaterialEditor",
            "BehaviorTreeEditor",
            "AIGraph"
        });

        DynamicallyLoadedModuleNames.AddRange(new string[] {
        });
    }
}
