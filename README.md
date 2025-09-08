LuaRuntime Plugin (UE 5.5)

Overview
- Safe, embeddable Lua 5.4 runtime for Shipping builds.
- Sandboxed by default: no file I/O, no OS, no dynamic library loading, and binary chunk loading disabled.
- Memory limit (allocator cap) and wall-clock timeout via instruction hook.
- Simple Blueprint API for running scripts and maintaining state.

Installation
- Place the plugin under `Plugins/LuaRuntime` in your project.
- Regenerate project files and build. The plugin vendors a slim subset of Lua sources; no external dependencies.

Blueprint API
- `LuaRuntimeSubsystem.CreateSandbox(MemoryLimitKB)` → returns `LuaSandbox` object.
- `LuaRuntimeSubsystem.ExecuteString(Code, MemoryLimitKB, TimeoutMs, HookInterval)` → run a one-off script in a fresh sandbox.
- `LuaSandbox.Initialize(MemoryLimitKB)` → init sandbox (auto-called by `CreateSandbox`).
- `LuaSandbox.RunString(Code, TimeoutMs, HookInterval)` → run code in that sandbox.
- `LuaSandbox.SetGlobalNumber/SetGlobalString` → expose values to Lua (as globals).
- `LuaSandbox.GetGlobalNumber/GetGlobalString` → read back globals.
- `LuaSandbox.Close()` → free the sandbox.

Safety
- Only safe libraries are opened: `base`, `table`, `string`, `math`, `utf8`, `coroutine`.
- Removed base functions: `dofile`, `loadfile`, and `load` (no file access, no binary chunks).
- `require` is unavailable (no `package` library is opened).
- Scripts run with a configurable wall-clock timeout and instruction-count hook; if exceeded, an error aborts execution.
- Custom allocator enforces a hard memory cap. Allocation beyond the cap fails gracefully with a Lua error.

Notes
- `print(...)` is replaced to log via UE (`LogLuaRuntime`).
- Binary chunks are disallowed; code is loaded in text-only mode.
- This initial version exposes a minimal API. You can add whitelisted native functions to the sandbox by pushing additional C functions into the Lua state in `ULuaSandbox::OpenSafeLibs()`.

Example
1) From Blueprint, call `CreateSandbox(1024)` then `RunString` with code:

```
print("Hello from Lua!", 2+2)
score = 42
```

2) Call `GetGlobalNumber("score")` to read back `42`.

3) For one-offs, call `ExecuteString` on the subsystem.

Extending
- To expose a whitelisted function, add a static C function in `LuaSandbox.cpp` and register it in `OpenSafeLibs()` (e.g., `lua_pushcfunction` + `lua_setglobal`). Keep the function side-effect free and validated.

License
- Lua is © PUC-Rio and distributed under the MIT license. See the upstream Lua distribution for details.

