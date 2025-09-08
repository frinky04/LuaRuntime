# Player Script System Example

## Overview
This example shows how to let players write Lua scripts that return numbers (or other values) in your game.

## Blueprint Setup

### Simple One-Shot Execution

1. **Add a Text Input Widget** for the player to write code
2. **Add a Button** to execute the script
3. **In the Button's OnClicked event:**

```
Get Game Instance
  ↓
Get Subsystem (LuaRuntimeSubsystem)
  ↓
Execute String
  - Code: [Text from input widget]
  - Memory Limit KB: 512
  - Timeout Ms: 100
  ↓
Branch (Success?)
  ↓
TRUE: Convert Return Value to Number → Display Result
FALSE: Display Error Message
```

## Example Player Scripts

### Basic Math
```lua
return 42
```
Returns: "42"

### Variables and Math
```lua
score = 4 + 4
bonus = 2
return score * bonus
```
Returns: "16"

### Using Functions
```lua
function calculate(a, b)
    return a * b + 10
end

return calculate(5, 3)
```
Returns: "25"

### Complex Calculations
```lua
-- Player's puzzle solution
width = 7
height = 6
answer = width * height
return answer
```
Returns: "42"

## Converting Return Values

The `ReturnValue` in `FLuaRunResult` is a string. To use it as a number in Blueprint:

1. **Check if Result.bSuccess is true**
2. **Use "String to Float" or "String to Int" node** to convert Result.ReturnValue
3. **Use the converted number in your game logic**

## Advanced: Persistent Script Environment

For a more advanced system where variables persist between script executions:

1. **Create a Named Sandbox** at game start:
   - `CreateNamedSandbox("PlayerScript", 1024)`

2. **When player submits code:**
   - `GetNamedSandbox("PlayerScript")`
   - `RunString` on that sandbox
   - Previous variables remain available

Example sequence:
```lua
-- First submission:
x = 10
return x
-- Returns: "10"

-- Second submission:
y = x * 2
return y
-- Returns: "20" (x is still 10 from before)

-- Third submission:
return x + y
-- Returns: "30"
```

## Error Handling

Always check `Result.bSuccess` before using the return value:

- **Success**: Display or use the `ReturnValue`
- **Failure**: Display `Result.Error` to help the player fix their script

Common errors:
- Syntax errors: "unexpected symbol near '...'"
- Runtime errors: "attempt to perform arithmetic on nil value"
- Timeout: "Execution timed out"

## Safety Features

The sandbox automatically prevents:
- File access
- Network access
- System commands
- Infinite loops (via timeout)
- Excessive memory usage

Players can safely experiment without risking the game or system.

## Blueprint Example: Score Calculator

1. **Create UI with:**
   - TextBox: "ScriptInput"
   - Button: "Calculate"
   - Text: "ResultDisplay"

2. **On Calculate Button Clicked:**
```
Get ScriptInput Text
  ↓
Execute String (from LuaRuntimeSubsystem)
  ↓
Branch on bSuccess
  ↓
Success:
  - String to Int (ReturnValue)
  - Set ResultDisplay: "Score: {value}"
Failure:
  - Set ResultDisplay: "Error: {Error}"
```

This gives players a safe, sandboxed environment to write scripts that return values your game can use!