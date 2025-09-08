# Repository Guidelines

## Project Structure & Module Organization
- `Source/LuaRuntime/Public` — Public headers exported via `LUARUNTIME_API`.
- `Source/LuaRuntime/Private` — Internal C++ implementation files.
- `ThirdParty/lua-5.4.7` — Vendored Lua source used by the plugin.
- `Content/` — Plugin assets (keep binary assets small; no secrets).
- `Binaries/` and `Intermediate/` — Generated build outputs; do not edit.
- `LuaRuntime.uplugin` — Plugin descriptor; update metadata and modules here.

## Build, Test, and Development Commands
- Build in Editor: Open your host project, enable the plugin, click `Compile`.
- Build with UAT (from Unreal Engine root):
  - Windows: `Engine/Build/BatchFiles/RunUAT.bat BuildPlugin -Plugin="<repo>/LuaRuntime.uplugin" -Package="<repo>/Binaries/Package" -TargetPlatforms=Win64`
  - macOS/Linux: `Engine/Build/BatchFiles/RunUAT.sh BuildPlugin -Plugin="<repo>/LuaRuntime.uplugin" -Package="<repo>/Binaries/Package" -TargetPlatforms=Mac,Linux`
- Clean (safe): remove `Binaries/` and `Intermediate/` within this plugin.

## Coding Style & Naming Conventions
- Follow Unreal Engine C++ style; keep changes consistent with existing code.
- Indentation: 4 spaces; UTF-8; Unix line endings preferred.
- Class prefixes: `U`/`A`/`S`/`F`/`I`; enums `EName`.
- File names match class names; headers in `Public`, sources in `Private`.
- Use `LUARUNTIME_API` for types exposed outside the module.
- Blueprint: annotate with `BlueprintCallable`, `BlueprintType`, etc., as needed.

## Testing Guidelines
- Prefer Unreal Automation Tests. Place test code in a separate test module or guarded sections.
- Run via Editor Session Frontend (Automation) or headless:
  - `UEEditor-Cmd <Project>.uproject -ExecCmds="Automation RunTests LuaRuntime" -unattended -nop4 -TestExit="Automation Test Queue Empty"`
- Aim for coverage of Lua bridging, sandboxing, and Blueprint nodes.

## Commit & Pull Request Guidelines
- Commits: short, imperative subject (e.g., `Fix Lua stack leak`). Optional scope: `Core:`, `Blueprints:`, `Build:`.
- PRs include: concise description, linked issues, test plan/steps, and screenshots for Blueprint changes.
- Keep diffs focused; update `README.md` and examples when behavior changes.

## Security & Configuration Tips
- Treat Lua as code: never execute untrusted scripts. Validate inputs at the C++ boundary.
- Do not commit large binaries or secrets. Respect the existing `.gitignore`.
- Prefer adding new configuration to `LuaRuntime.uplugin` rather than hardcoding paths.

