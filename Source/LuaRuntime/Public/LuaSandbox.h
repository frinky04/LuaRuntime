#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaSandbox.generated.h"

struct lua_State;

USTRUCT(BlueprintType)
struct FLuaRunResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bSuccess = false;

    UPROPERTY(BlueprintReadOnly)
    FString Error;
};

UCLASS(BlueprintType)
class LUARUNTIME_API ULuaSandbox : public UObject
{
    GENERATED_BODY()

public:
    ULuaSandbox();
    virtual void BeginDestroy() override;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void Initialize(int32 MemoryLimitKB = 1024);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    FLuaRunResult RunString(const FString& Code, int32 TimeoutMs = 50, int32 HookInterval = 1000);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void SetGlobalNumber(const FName Name, double Value);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void SetGlobalString(const FName Name, const FString& Value);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool GetGlobalNumber(const FName Name, double& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool GetGlobalString(const FName Name, FString& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void Close();

private:
    void* CreateState(int32 MemoryLimitKB);
    void OpenSafeLibs();
    void InstallPrint();
    void RemoveUnsafeBaseFuncs();

private:
    lua_State* L = nullptr;
    int64 AllocLimitBytes = 0;
};

