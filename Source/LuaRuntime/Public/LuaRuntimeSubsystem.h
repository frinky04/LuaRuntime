#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LuaSandbox.h"
#include "LuaRuntimeSubsystem.generated.h"

UCLASS()
class LUARUNTIME_API ULuaRuntimeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    ULuaSandbox* CreateSandbox(int32 MemoryLimitKB = 1024);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    FLuaRunResult ExecuteString(const FString& Code, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50, int32 HookInterval = 1000);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Create Named Sandbox"))
    ULuaSandbox* CreateNamedSandbox(const FName SandboxName, int32 MemoryLimitKB = 1024);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Get Named Sandbox"))
    ULuaSandbox* GetNamedSandbox(const FName SandboxName) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Remove Named Sandbox"))
    bool RemoveNamedSandbox(const FName SandboxName);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    TArray<FName> GetAllSandboxNames() const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Execute File"))
    FLuaRunResult ExecuteFile(const FString& FilePath, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50, int32 HookInterval = 1000);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Evaluate Expression"))
    FLuaRunResult EvaluateExpression(const FString& Expression, int32 MemoryLimitKB = 1024, int32 TimeoutMs = 50);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void ClearAllSandboxes();

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Validate Lua Syntax"))
    bool ValidateLuaSyntax(const FString& Code, FString& OutError) const;

private:
    UPROPERTY()
    TMap<FName, ULuaSandbox*> NamedSandboxes;
};

