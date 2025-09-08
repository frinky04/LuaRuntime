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
};

