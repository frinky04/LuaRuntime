# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is an Unreal Engine 5.5 plugin that provides a safe, sandboxed Lua 5.4 runtime for use in shipping builds. The plugin embeds a subset of Lua sources and exposes Blueprint-accessible APIs for running Lua scripts with memory and execution time limits.

## Build and Development Commands

### Building the Plugin
- This is an Unreal Engine plugin - it must be built as part of an Unreal project
- Regenerate project files: In your UE project root, right-click the `.uproject` file and select "Generate Visual Studio project files" (or use the UnrealBuildTool command line)
- Build via Visual Studio or command line using UnrealBuildTool
- The plugin uses the Unreal Build System (UBT) configured in `Source/LuaRuntime/LuaRuntime.Build.cs`

### Testing
- Test in Unreal Editor using Blueprint nodes exposed by `ULuaRuntimeSubsystem` and `ULuaSandbox`
- Create test Blueprints that call `CreateSandbox()` and `RunString()` methods
- Check Output Log for Lua print statements (they log to `LogLuaRuntime` category)

## Architecture

### Core Components

1. **ULuaRuntimeSubsystem** (`Public/LuaRuntimeSubsystem.h`)
   - Game instance subsystem providing the main entry point
   - Creates sandboxed Lua environments
   - Provides one-off script execution via `ExecuteString()`

2. **ULuaSandbox** (`Public/LuaSandbox.h`, `Private/LuaSandbox.cpp`)
   - Manages individual Lua state instances
   - Implements memory allocation tracking and limits
   - Provides timeout enforcement via instruction hooks
   - Exposes Blueprint-callable methods for script execution and variable exchange
   - Key safety features implemented in `OpenSafeLibs()` and `RemoveUnsafeBaseFuncs()`

3. **Lua Integration**
   - Slim Lua sources in `Source/LuaRuntime/Private/ThirdParty/lua_slim/src/`
   - Full Lua headers in `ThirdParty/lua-5.4.7/src/`
   - Custom allocator with memory tracking in `LuaSandbox.cpp`
   - Instruction hook for timeout enforcement

### Safety Architecture

The sandbox removes potentially dangerous Lua functionality:
- No file I/O (`io` library not loaded)
- No OS access (`os` library not loaded)
- No dynamic library loading (`package` library not loaded)
- No binary chunk loading (text-only mode enforced)
- Removed base functions: `dofile`, `loadfile`, `load`
- Memory allocation capped via custom allocator
- Execution time limited via instruction counting hook

### Extending the Plugin

To add new whitelisted functions:
1. Add static C function in `LuaSandbox.cpp`
2. Register in `ULuaSandbox::OpenSafeLibs()` using `lua_pushcfunction` + `lua_setglobal`
3. Ensure functions are side-effect free and properly validated

## Module Dependencies

From `LuaRuntime.Build.cs`:
- Public: `Core`
- Private: `CoreUObject`, `Engine`, `Slate`, `SlateCore`
- Include paths configured for both slim Lua sources and full headers

## Important Files

- `LuaRuntime.uplugin` - Plugin descriptor
- `Source/LuaRuntime/LuaRuntime.Build.cs` - Build configuration
- `Source/LuaRuntime/Private/LuaSandbox.cpp` - Core sandbox implementation with safety features
- `README.md` - User-facing documentation with Blueprint API reference