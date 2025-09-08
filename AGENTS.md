# Repository Guidelines

## Project Structure & Module Organization
- `Source/LuaRuntime/Public` — Public headers exported with `LUARUNTIME_API`.
- `Source/LuaRuntime/Private` — Internal C++ implementation files.
- `ThirdParty/lua-5.4.7` — Vendored Lua sources used by the plugin.
- `Content/` — Plugin assets (keep binary assets small; no secrets).
- `Binaries/`, `Intermediate/` — Generated output; do not edit.
- `LuaRuntime.uplugin` — Plugin descriptor; update metadata and modules here.

## Build, Test, and Development Commands
- Build in Editor: enable the plugin in your host project, then click `Compile`.
- Package with UAT (from Unreal Engine root):
  - Windows: `Engine/Build/BatchFiles/RunUAT.bat BuildPlugin -Plugin="<repo>/LuaRuntime.uplugin" -Package="<repo>/Binaries/Package" -TargetPlatforms=Win64`
  - macOS/Linux: `Engine/Build/BatchFiles/RunUAT.sh BuildPlugin -Plugin="<repo>/LuaRuntime.uplugin" -Package="<repo>/Binaries/Package" -TargetPlatforms=Mac,Linux`
- Clean: remove `Binaries/` and `Intermediate/` inside this plugin directory.

## Coding Style & Naming Conventions
- Follow Unreal Engine C++ style; 4‑space indents, UTF‑8, Unix line endings.
- Class prefixes `U/A/S/F/I`; enums `EName`. Filenames match class names.
- Public APIs use `LUARUNTIME_API`. Place headers in `Public`, sources in `Private`.
- Blueprint exposure: annotate with `BlueprintCallable`, `BlueprintType`, etc., where appropriate.

## Testing Guidelines
- Prefer Unreal Automation Tests. Place tests in a separate test module or guard them from shipping builds.
- Run via Editor: Session Frontend → Automation.
- Headless example: `UEEditor-Cmd <Project>.uproject -ExecCmds="Automation RunTests LuaRuntime" -unattended -nop4 -TestExit="Automation Test Queue Empty"`.

## Commit & Pull Request Guidelines
- Commits: short, imperative subject (e.g., `Fix Lua stack leak`). Optional scope prefix (`Core:`, `Blueprints:`, `Build:`).
- PRs include: concise description, linked issues, test plan/steps, and screenshots for Blueprint/UI changes. Keep diffs focused; update `README.md` and examples when behavior changes.

## Security & Configuration Tips
- Treat Lua as code: never execute untrusted scripts; validate inputs at C++ boundaries.
- Don’t commit large binaries or secrets; respect `.gitignore`.
- Prefer configuration in `LuaRuntime.uplugin` over hardcoded paths or flags.

