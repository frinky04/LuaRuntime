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
