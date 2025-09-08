# New Blueprint Interfaces for Lua Runtime

## Overview
The Lua Runtime plugin has been enhanced with additional Blueprint-accessible functionality to provide more powerful and flexible Lua scripting capabilities in Unreal Engine 5.5.

## New Data Structures

### FLuaValue
A flexible structure for passing values between Blueprint and Lua:
- `StringValue` - String representation
- `NumberValue` - Numeric value (double)
- `BoolValue` - Boolean value
- `bIsNil` - Indicates if the value is nil/undefined

## Enhanced ULuaSandbox Methods

### Boolean Operations
- **SetGlobalBool** - Set a global boolean variable in Lua
- **GetGlobalBool** - Get a global boolean variable from Lua

### Function Calling
- **CallFunction** - Call a Lua function with arguments and get return value
  - Pass function name and array of FLuaValue arguments
  - Returns FLuaRunResult with success status, error message, and return value

### Global Variable Management
- **HasGlobal** - Check if a global variable exists
- **ClearGlobal** - Remove a global variable
- **GetGlobalNames** - Get list of all global variable names

### Table Operations
- **SetTableValue** - Set a value in a Lua table using dot notation (e.g., "player.stats.health")
- **GetTableValue** - Get a value from a Lua table using dot notation

### File Operations
- **RunFile** - Execute a Lua script from a file path

### Blueprint Callbacks
- **RegisterCallback** - Register a callback that Lua can call to trigger Blueprint events
- **OnLuaCallback** - Blueprint event that fires when Lua calls a registered callback

### Memory Management
- **GetMemoryUsage** - Get current memory usage in bytes
- **SetMemoryLimit** - Change the memory limit at runtime

### Expression Evaluation
- **EvaluateExpression** - Evaluate a Lua expression and return the result

## Enhanced ULuaRuntimeSubsystem Methods

### Named Sandbox Management
- **CreateNamedSandbox** - Create a sandbox with a specific name for persistent access
- **GetNamedSandbox** - Retrieve a previously created named sandbox
- **RemoveNamedSandbox** - Remove and cleanup a named sandbox
- **GetAllSandboxNames** - Get list of all named sandbox identifiers
- **ClearAllSandboxes** - Remove all named sandboxes

### File and Expression Utilities
- **ExecuteFile** - Execute a Lua script file in a temporary sandbox
- **EvaluateExpression** - Evaluate a Lua expression in a temporary sandbox

### Validation
- **ValidateLuaSyntax** - Check if Lua code has valid syntax without executing it

## Usage Examples in Blueprint

### Example 1: Persistent Game State
1. Create a named sandbox for game state: `CreateNamedSandbox("GameState", 2048)`
2. Set initial values: `SetGlobalNumber("score", 0)`, `SetGlobalString("playerName", "Hero")`
3. Run game logic scripts that modify the state
4. Retrieve values later: `GetGlobalNumber("score")`

### Example 2: Dynamic Function Calls
1. Create sandbox and load script with functions
2. Prepare arguments as array of FLuaValue
3. Call function: `CallFunction("calculateDamage", Args)`
4. Process return value from FLuaRunResult

### Example 3: Blueprint Callbacks from Lua
1. Register callback: `RegisterCallback("onEnemyDefeated")`
2. Bind to OnLuaCallback event in Blueprint
3. In Lua: `onEnemyDefeated("BossName", 1000)` triggers the Blueprint event

### Example 4: Table Manipulation
1. Run script that creates tables: `RunString("player = {stats = {health = 100, mana = 50}}")`
2. Get nested value: `GetTableValue("player.stats", "health")`
3. Set nested value: `SetTableValue("player.stats", "health", NewHealthValue)`

### Example 5: Syntax Validation
1. Before running user-provided code: `ValidateLuaSyntax(UserCode, ErrorMessage)`
2. If valid, execute the code
3. If invalid, display error to user

## Safety Considerations
All new methods maintain the sandboxing safety features:
- Memory limits are enforced
- Execution timeouts prevent infinite loops
- File I/O remains disabled within Lua scripts
- OS access remains blocked
- Dynamic library loading remains prohibited

## Performance Tips
1. Use named sandboxes for persistent state instead of creating new sandboxes repeatedly
2. Batch operations when possible (set multiple globals before running scripts)
3. Use EvaluateExpression for simple calculations instead of full script execution
4. Validate syntax before execution for user-provided code
5. Monitor memory usage with GetMemoryUsage() for long-running sandboxes