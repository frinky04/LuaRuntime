// Copyright Epic Games, Inc. All Rights Reserved.

#include "LuaRuntime.h"

#define LOCTEXT_NAMESPACE "FLuaRuntimeModule"

DEFINE_LOG_CATEGORY(LogLuaRuntime);

void FLuaRuntimeModule::StartupModule()
{
	// Initialize anything global to the module here if needed
}

void FLuaRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FLuaRuntimeModule, LuaRuntime)
