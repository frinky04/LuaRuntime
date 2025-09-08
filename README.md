# LuaRuntime Plugin (UE 5.5)

## Overview
- Safe, embeddable Lua 5.4 runtime for Shipping builds.
- Sandboxed by default: no file I/O, no OS, no dynamic library loading, and binary chunk loading disabled.
- Memory limit (allocator cap) and wall-clock timeout via instruction hook.
- Simple Blueprint API for running scripts and maintaining state.
- Editor integration: Lua Script asset type + Project Settings panel.

## Installation
- Place the plugin under `Plugins/LuaRuntime` in your project.
- Regenerate project files and build. The plugin vendors a slim subset of Lua sources; no external dependencies.

## Blueprint API

### Core Subsystem Methods
- `LuaRuntimeSubsystem.CreateSandbox(MemoryLimitKB)` → returns `LuaSandbox` object.
- `LuaRuntimeSubsystem.ExecuteString(Code, MemoryLimitKB, TimeoutMs, HookInterval)` → run a one-off script in a fresh sandbox, returns `FLuaRunResult` with success status, error message, and return value.
- `LuaRuntimeSubsystem.CreateNamedSandbox(Name, MemoryLimitKB)` → create a persistent named sandbox.
- `LuaRuntimeSubsystem.GetNamedSandbox(Name)` → retrieve a previously created named sandbox.
- `LuaRuntimeSubsystem.RemoveNamedSandbox(Name)` → remove and cleanup a named sandbox.
- `LuaRuntimeSubsystem.ExecuteFile(FilePath, MemoryLimitKB, TimeoutMs, HookInterval)` → execute a Lua script from file.
- `LuaRuntimeSubsystem.EvaluateExpression(Expression, MemoryLimitKB, TimeoutMs)` → evaluate a Lua expression and return result.
- `LuaRuntimeSubsystem.ValidateLuaSyntax(Code, OutError)` → check syntax without execution.
- `LuaRuntimeSubsystem.ClearAllSandboxes()` → remove all named sandboxes.

### Blueprint Library Shortcuts
- `Execute Lua Chunk(WorldContext, Code, MemoryKB, TimeoutMs, HookInterval)` → convenience wrapper for one‑off code.
- `Execute Lua File(WorldContext, FilePath, MemoryKB, TimeoutMs, HookInterval)` → loads file then executes.
- `Evaluate Lua Expression(WorldContext, Expression, MemoryKB, TimeoutMs)` → returns expression value.
- `Execute Lua Script Asset(WorldContext, ULuaScript)` → run a `ULuaScript` asset.
- `Get Lua Runtime Subsystem(WorldContext)` → fetch the subsystem quickly.

### Sandbox Methods
- `LuaSandbox.Initialize(MemoryLimitKB)` → init sandbox (auto-called by `CreateSandbox`).
- `LuaSandbox.RunString(Code, TimeoutMs, HookInterval)` → run code in that sandbox, returns `FLuaRunResult` with return value.
- `LuaSandbox.SetGlobalNumber/SetGlobalString/SetGlobalBool` → expose values to Lua (as globals).
- `LuaSandbox.GetGlobalNumber/GetGlobalString/GetGlobalBool` → read back globals.
- `LuaSandbox.CallFunction(FunctionName, Args, TimeoutMs)` → call a Lua function with arguments.
- `LuaSandbox.HasGlobal(Name)` → check if a global variable exists.
- `LuaSandbox.ClearGlobal(Name)` → remove a global variable.
- `LuaSandbox.GetGlobalNames()` → get list of all global variable names.
- `LuaSandbox.SetTableValue(TablePath, Key, Value)` → set a value in a Lua table using dot notation.
- `LuaSandbox.GetTableValue(TablePath, Key, OutValue)` → get a value from a Lua table.
- `LuaSandbox.RunFile(FilePath, TimeoutMs, HookInterval)` → execute a Lua script from file.
- `LuaSandbox.RegisterCallback(CallbackName)` → register a Blueprint callback that Lua can invoke.
- `LuaSandbox.GetMemoryUsage()` → get current memory usage in bytes.
- `LuaSandbox.SetMemoryLimit(NewLimitKB)` → change memory limit at runtime.
- `LuaSandbox.EvaluateExpression(Expression, TimeoutMs)` → evaluate and return expression result.
- `LuaSandbox.Close()` → free the sandbox.

### Data Structures
- `FLuaRunResult` → Contains `bSuccess` (bool), `Error` (FString), and `ReturnValue` (FString).
- `FLuaValue` → Flexible value type with `StringValue`, `NumberValue`, `BoolValue`, and `bIsNil` fields.
- `OnLuaCallback` → Blueprint event delegate that fires when Lua calls a registered callback.

## Actor Component
- `ULuaComponent` can be added to any Actor.
  - Configure to run a File path, a `ULuaScript` asset, or Inline code.
  - Optional Named Sandbox to share state across actors.
  - Exposes `ExecuteConfiguredScript()` and `CallLuaFunction()` utilities.

## Editor Integration & Assets
- Lua Script Asset: Content Browser → Add → Miscellaneous → Lua Script.
  - Edit `Source` text; click Validate Syntax (auto‑validates on edit).
  - Use in Blueprints via `Execute Lua Script Asset` or via `ULuaComponent`.

## Project Settings
- Project Settings → Plugins → LuaRuntime
  - Printing
    - On‑Screen Print Enabled (default on)
    - On‑Screen Print Duration (seconds)
    - On‑Screen Print Color
    - Allow On‑Screen Print In Shipping (default off)
  - Execution Defaults
    - Default Memory Limit (KB)
    - Default Timeout (ms)
    - Default Hook Interval (instructions)

## Safety
- Only safe libraries are opened: `base`, `table`, `string`, `math`, `utf8`, `coroutine`.
- Removed base functions: `dofile`, `loadfile`, and `load` (no file access, no binary chunks).
- `require` is unavailable (no `package` library is opened).
- Scripts run with a configurable wall-clock timeout and instruction-count hook; if exceeded, an error aborts execution.
- Custom allocator enforces a hard memory cap. Allocation beyond the cap fails gracefully with a Lua error.

## Notes
- `print(...)` logs via UE (`LogLuaRuntime`) and, if enabled in settings, shows an on‑screen message.
- Binary chunks are disallowed; code is loaded in text-only mode.
- This initial version exposes a minimal API. You can add whitelisted native functions to the sandbox by pushing additional C functions into the Lua state in `ULuaSandbox::OpenSafeLibs()`.

## Examples

### Basic Script Execution
1) From Blueprint, call `CreateSandbox(1024)` then `RunString` with code:
```lua
print("Hello from Lua!", 2+2)
score = 42
```
2) Call `GetGlobalNumber("score")` to read back `42`.

### Return Values from Scripts
Execute a script that returns a value:
```lua
result = 10 * 4 + 2
return result
```
The `FLuaRunResult.ReturnValue` will contain "42" as a string.

### Function Calling
```lua
function multiply(a, b)
    return a * b
end
```
Then from Blueprint, prepare an array of `FLuaValue` arguments and call:
`CallFunction("multiply", [5, 8])` → Returns "40" in the result.

### Named Sandboxes for Persistent State
```blueprint
CreateNamedSandbox("GameState", 2048)
GetNamedSandbox("GameState")
→ RunString("level = 1; score = 0")
→ Later: GetGlobalNumber("score")
```

### Table Manipulation
```lua
player = {
    stats = {
        health = 100,
        mana = 50
    }
}
```
Then use `GetTableValue("player.stats", "health")` to retrieve nested values.

### Blueprint Callbacks
Register a callback: `RegisterCallback("onEvent")`
From Lua: `onEvent("player_died", 100)` → Triggers the `OnLuaCallback` Blueprint event.

## Extending
- To expose a whitelisted function, add a static C function in `LuaSandbox.cpp` and register it in `OpenSafeLibs()` (e.g., `lua_pushcfunction` + `lua_setglobal`). Keep the function side-effect free and validated.

## License
- Lua is © PUC-Rio and distributed under the MIT license. See the upstream Lua distribution for details.
