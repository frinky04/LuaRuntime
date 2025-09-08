#include "LuaRuntimeSubsystem.h"
#include "LuaSandbox.h"

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

