#include "LuaRuntimeSubsystem.h"
#include "LuaSandbox.h"
#include "Misc/FileHelper.h"

// Lua headers for syntax validation
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

ULuaSandbox* ULuaRuntimeSubsystem::CreateSandbox(int32 MemoryLimitKB)
{
    ULuaSandbox* Box = NewObject<ULuaSandbox>(this);
    Box->Initialize(MemoryLimitKB);
    return Box;
}

FLuaRunResult ULuaRuntimeSubsystem::ExecuteString(const FString& Code, int32 MemoryLimitKB, int32 TimeoutMs, int32 HookInterval)
{
    ULuaSandbox* Box = CreateSandbox(MemoryLimitKB);
    if (!Box)
    {
        FLuaRunResult R; R.bSuccess = false; R.Error = TEXT("Failed to create sandbox"); return R;
    }
    const FLuaRunResult R = Box->RunString(Code, TimeoutMs, HookInterval);
    Box->Close();
    return R;
}

ULuaSandbox* ULuaRuntimeSubsystem::CreateNamedSandbox(const FName SandboxName, int32 MemoryLimitKB)
{
    if (NamedSandboxes.Contains(SandboxName))
    {
        ULuaSandbox* ExistingBox = NamedSandboxes[SandboxName];
        if (ExistingBox)
        {
            ExistingBox->Close();
        }
    }

    ULuaSandbox* Box = NewObject<ULuaSandbox>(this);
    Box->Initialize(MemoryLimitKB);
    NamedSandboxes.Add(SandboxName, Box);
    return Box;
}

ULuaSandbox* ULuaRuntimeSubsystem::GetNamedSandbox(const FName SandboxName) const
{
    if (NamedSandboxes.Contains(SandboxName))
    {
        return NamedSandboxes[SandboxName];
    }
    return nullptr;
}

bool ULuaRuntimeSubsystem::RemoveNamedSandbox(const FName SandboxName)
{
    if (NamedSandboxes.Contains(SandboxName))
    {
        ULuaSandbox* Box = NamedSandboxes[SandboxName];
        if (Box)
        {
            Box->Close();
        }
        NamedSandboxes.Remove(SandboxName);
        return true;
    }
    return false;
}

TArray<FName> ULuaRuntimeSubsystem::GetAllSandboxNames() const
{
    TArray<FName> Names;
    NamedSandboxes.GetKeys(Names);
    return Names;
}

FLuaRunResult ULuaRuntimeSubsystem::ExecuteFile(const FString& FilePath, int32 MemoryLimitKB, int32 TimeoutMs, int32 HookInterval)
{
    FLuaRunResult Result;
    
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *FilePath))
    {
        Result.bSuccess = false;
        Result.Error = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
        return Result;
    }

    return ExecuteString(Content, MemoryLimitKB, TimeoutMs, HookInterval);
}

FLuaRunResult ULuaRuntimeSubsystem::EvaluateExpression(const FString& Expression, int32 MemoryLimitKB, int32 TimeoutMs)
{
    FString EvalCode = FString::Printf(TEXT("return %s"), *Expression);
    return ExecuteString(EvalCode, MemoryLimitKB, TimeoutMs, 1000);
}

void ULuaRuntimeSubsystem::ClearAllSandboxes()
{
    for (auto& Pair : NamedSandboxes)
    {
        if (Pair.Value)
        {
            Pair.Value->Close();
        }
    }
    NamedSandboxes.Empty();
}

bool ULuaRuntimeSubsystem::ValidateLuaSyntax(const FString& Code, FString& OutError) const
{
    lua_State* L = luaL_newstate();
    if (!L)
    {
        OutError = TEXT("Failed to create Lua state for validation");
        return false;
    }

    FTCHARToUTF8 CodeUtf8(*Code);
    int loadStatus = luaL_loadbufferx(L, CodeUtf8.Get(), CodeUtf8.Length(), "validation", "t");
    
    if (loadStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        OutError = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown syntax error");
        lua_close(L);
        return false;
    }

    lua_close(L);
    OutError.Empty();
    return true;
}

