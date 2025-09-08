#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaSandbox.h"
#include "LuaRuntimeSubsystem.h"
#include "LuaBlueprintLibrary.generated.h"

UCLASS()
class LUARUNTIME_API ULuaBlueprintLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="Lua", meta=(WorldContext="WorldContextObject"))
    static ULuaRuntimeSubsystem* GetLuaRuntimeSubsystem(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category="Lua", meta=(WorldContext="WorldContextObject", DisplayName="Execute Lua Chunk"))
    static FLuaRunResult ExecuteLuaChunk(const UObject* WorldContextObject, const FString& Code, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50, int32 HookInterval = 1000);

    UFUNCTION(BlueprintCallable, Category="Lua", meta=(WorldContext="WorldContextObject", DisplayName="Execute Lua File"))
    static FLuaRunResult ExecuteLuaFile(const UObject* WorldContextObject, const FString& FilePath, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50, int32 HookInterval = 1000);

    UFUNCTION(BlueprintCallable, Category="Lua", meta=(WorldContext="WorldContextObject", DisplayName="Evaluate Lua Expression"))
    static FLuaRunResult EvaluateLuaExpression(const UObject* WorldContextObject, const FString& Expression, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50);

    UFUNCTION(BlueprintCallable, Category="Lua", meta=(WorldContext="WorldContextObject", DisplayName="Execute Lua Script Asset"))
    static FLuaRunResult ExecuteLuaScriptAsset(const UObject* WorldContextObject, class ULuaScript* Script, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50, int32 HookInterval = 1000);
};
