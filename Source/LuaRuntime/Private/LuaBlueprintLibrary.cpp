#include "LuaBlueprintLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "LuaScript.h"

ULuaRuntimeSubsystem* ULuaBlueprintLibrary::GetLuaRuntimeSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }
    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }
    if (UGameInstance* GI = World->GetGameInstance())
    {
        return GI->GetSubsystem<ULuaRuntimeSubsystem>();
    }
    return nullptr;
}

FLuaRunResult ULuaBlueprintLibrary::ExecuteLuaChunk(const UObject* WorldContextObject, const FString& Code, int32 MemoryLimitKB, int32 TimeoutMs, int32 HookInterval)
{
    if (ULuaRuntimeSubsystem* Subsys = GetLuaRuntimeSubsystem(WorldContextObject))
    {
        return Subsys->ExecuteString(Code, MemoryLimitKB, TimeoutMs, HookInterval);
    }
    FLuaRunResult R; R.bSuccess = false; R.Error = TEXT("LuaRuntimeSubsystem not available"); return R;
}

FLuaRunResult ULuaBlueprintLibrary::ExecuteLuaFile(const UObject* WorldContextObject, const FString& FilePath, int32 MemoryLimitKB, int32 TimeoutMs, int32 HookInterval)
{
    if (ULuaRuntimeSubsystem* Subsys = GetLuaRuntimeSubsystem(WorldContextObject))
    {
        return Subsys->ExecuteFile(FilePath, MemoryLimitKB, TimeoutMs, HookInterval);
    }
    FLuaRunResult R; R.bSuccess = false; R.Error = TEXT("LuaRuntimeSubsystem not available"); return R;
}

FLuaRunResult ULuaBlueprintLibrary::EvaluateLuaExpression(const UObject* WorldContextObject, const FString& Expression, int32 MemoryLimitKB, int32 TimeoutMs)
{
    if (ULuaRuntimeSubsystem* Subsys = GetLuaRuntimeSubsystem(WorldContextObject))
    {
        return Subsys->EvaluateExpression(Expression, MemoryLimitKB, TimeoutMs);
    }
    FLuaRunResult R; R.bSuccess = false; R.Error = TEXT("LuaRuntimeSubsystem not available"); return R;
}

FLuaRunResult ULuaBlueprintLibrary::ExecuteLuaScriptAsset(const UObject* WorldContextObject, ULuaScript* Script, int32 MemoryLimitKB, int32 TimeoutMs, int32 HookInterval)
{
    if (!Script)
    {
        FLuaRunResult R; R.bSuccess = false; R.Error = TEXT("Script asset is null"); return R;
    }
    if (ULuaRuntimeSubsystem* Subsys = GetLuaRuntimeSubsystem(WorldContextObject))
    {
        return Subsys->ExecuteString(Script->GetSource(), MemoryLimitKB, TimeoutMs, HookInterval);
    }
    FLuaRunResult R; R.bSuccess = false; R.Error = TEXT("LuaRuntimeSubsystem not available"); return R;
}

bool ULuaBlueprintLibrary::ExecuteLuaChunkDyn(const UObject* WorldContextObject, const FString& Code, FLuaDynValue& OutValue, FString& OutError, int32 MemoryLimitKB, int32 TimeoutMs, int32 HookInterval)
{
    OutError.Empty();
    if (ULuaRuntimeSubsystem* Subsys = GetLuaRuntimeSubsystem(WorldContextObject))
    {
        ULuaSandbox* Box = Subsys->CreateSandbox(MemoryLimitKB);
        if (!Box) { OutError = TEXT("Failed to create sandbox"); return false; }
        const bool bOk = Box->RunStringDyn(Code, TimeoutMs, HookInterval, OutValue, OutError);
        Box->Close();
        return bOk;
    }
    OutError = TEXT("LuaRuntimeSubsystem not available");
    return false;
}

bool ULuaBlueprintLibrary::EvaluateLuaExpressionDyn(const UObject* WorldContextObject, const FString& Expression, FLuaDynValue& OutValue, FString& OutError, int32 MemoryLimitKB, int32 TimeoutMs)
{
    OutError.Empty();
    if (ULuaRuntimeSubsystem* Subsys = GetLuaRuntimeSubsystem(WorldContextObject))
    {
        ULuaSandbox* Box = Subsys->CreateSandbox(MemoryLimitKB);
        if (!Box) { OutError = TEXT("Failed to create sandbox"); return false; }
        const bool bOk = Box->EvaluateExpressionDyn(Expression, TimeoutMs, OutValue, OutError);
        Box->Close();
        return bOk;
    }
    OutError = TEXT("LuaRuntimeSubsystem not available");
    return false;
}
