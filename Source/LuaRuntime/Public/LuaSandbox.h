#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaValue.h"
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

    UPROPERTY(BlueprintReadOnly)
    FString ReturnValue;
};

USTRUCT(BlueprintType)
struct FLuaValue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    FString StringValue;

    UPROPERTY(BlueprintReadWrite)
    double NumberValue = 0.0;

    UPROPERTY(BlueprintReadWrite)
    bool BoolValue = false;

    UPROPERTY(BlueprintReadWrite)
    bool bIsNil = true;
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

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void SetGlobalBool(const FName Name, bool Value);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool GetGlobalBool(const FName Name, bool& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void SetGlobalDyn(const FName Name, const FLuaDynValue& Value);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool GetGlobalDyn(const FName Name, FLuaDynValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Call Lua Function"))
    FLuaRunResult CallFunction(const FString& FunctionName, const TArray<FLuaValue>& Args, int32 TimeoutMs = 50);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta=(DisplayName="Call Lua Function (Dyn)") )
    FLuaRunResult CallFunctionDyn(const FString& FunctionName, const TArray<FLuaDynValue>& Args, int32 TimeoutMs = 50);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool HasGlobal(const FName Name) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void ClearGlobal(const FName Name);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    TArray<FString> GetGlobalNames() const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Set Table Value"))
    bool SetTableValue(const FString& TablePath, const FString& Key, const FLuaValue& Value);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Set Table Value (Dyn)"))
    bool SetTableValueDyn(const FString& TablePath, const FString& Key, const FLuaDynValue& Value);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Get Table Value"))
    bool GetTableValue(const FString& TablePath, const FString& Key, FLuaValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Get Table Value (Dyn)"))
    bool GetTableValueDyn(const FString& TablePath, const FString& Key, FLuaDynValue& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    FLuaRunResult RunFile(const FString& FilePath, int32 TimeoutMs = 50, int32 HookInterval = 1000);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool RunStringDyn(const FString& Code, int32 TimeoutMs, int32 HookInterval, FLuaDynValue& OutValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    bool RunFileDyn(const FString& FilePath, int32 TimeoutMs, int32 HookInterval, FLuaDynValue& OutValue, FString& OutError);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Register Blueprint Callback"))
    void RegisterCallback(const FString& CallbackName);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    int64 GetMemoryUsage() const;

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime")
    void SetMemoryLimit(int32 NewLimitKB);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Evaluate Expression"))
    FLuaRunResult EvaluateExpression(const FString& Expression, int32 TimeoutMs = 50);

    UFUNCTION(BlueprintCallable, Category = "LuaRuntime", meta = (DisplayName = "Evaluate Expression (Dyn)"))
    bool EvaluateExpressionDyn(const FString& Expression, int32 TimeoutMs, FLuaDynValue& OutValue, FString& OutError);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLuaCallback, const FString&, CallbackName, const TArray<FLuaValue>&, Args);
    UPROPERTY(BlueprintAssignable, Category = "LuaRuntime")
    FOnLuaCallback OnLuaCallback;

private:
    void* CreateState(int32 MemoryLimitKB);
    void OpenSafeLibs();
    void InstallPrint();
    void RemoveUnsafeBaseFuncs();
    void PushLuaValue(const FLuaValue& Value);
    void PushLuaDynValue(const FLuaDynValue& Value);
    FLuaValue PopLuaValue() const;
    FLuaDynValue PopLuaDynValue() const;
    bool GetTableByPath(const FString& TablePath) const;

private:
    lua_State* L = nullptr;
    int64 AllocLimitBytes = 0;
};
