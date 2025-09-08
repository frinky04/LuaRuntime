// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LuaRuntime : ModuleRules
{
	public LuaRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// Public headers
			}
			);
				
		
        PrivateIncludePaths.AddRange(
            new string[] {
                // Lua slim sources (compiled into this module)
                System.IO.Path.Combine(ModuleDirectory, "Private", "ThirdParty", "lua_slim", "src"),
                // Full Lua headers (do not compile sources here)
                System.IO.Path.Combine(ModuleDirectory, "..", "..", "ThirdParty", "lua-5.4.7", "src"),
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
			}
		);

		// Treat C files as C and not C++ where applicable
		bEnableUndefinedIdentifierWarnings = false;
		bUseRTTI = false;
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
