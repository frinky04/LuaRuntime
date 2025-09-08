#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LuaSandbox.h"
#include "LuaComponent.generated.h"

/**
 * ULuaComponent binds an Actor to a Lua sandbox and can run a file or inline code on BeginPlay.
 */
UCLASS(ClassGroup=(Lua), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class LUARUNTIME_API ULuaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    ULuaComponent();

    // Begin UActorComponent
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // End UActorComponent

public: // Configuration
    /** If true, the component runs the configured script at BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    bool bAutoRunOnBeginPlay = true;

    /** If true, run from file path; otherwise use asset or inline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    bool bUseFile = true;

    /** If true and not using file, execute from ScriptAsset. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="!bUseFile"))
    bool bUseAsset = false;

    /** Absolute or project-relative file path to a Lua script. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="bUseFile"))
    FString ScriptFilePath;

    /** Inline Lua code to execute (used when not using file nor asset). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(MultiLine=true, EditCondition="!bUseFile && !bUseAsset"))
    FString ScriptCode;

    /** Lua script asset to execute (used when bUseFile=false and bUseAsset=true). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="!bUseFile && bUseAsset"))
    class ULuaScript* ScriptAsset;

    /** Memory limit for the sandbox in KB. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(ClampMin="64", UIMin="64"))
    int32 MemoryLimitKB = 1024;

    /** Timeout in milliseconds for execution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(ClampMin="1", UIMin="1"))
    int32 TimeoutMs = 50;

    /** Hook instruction interval for time slicing (lower = more checks). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(ClampMin="1", UIMin="1"))
    int32 HookInterval = 1000;

    /** Optional: create or use a named sandbox via the subsystem. Empty = private sandbox. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    FName NamedSandbox;

public: // Runtime API
    /** Ensure a sandbox exists (named via subsystem or private) and return it. */
    UFUNCTION(BlueprintCallable, Category="Lua")
    ULuaSandbox* EnsureSandbox();

    /** Execute the configured script (file or inline). */
    UFUNCTION(BlueprintCallable, Category="Lua")
    FLuaRunResult ExecuteConfiguredScript();

    /** Call a Lua function within this component's sandbox. */
    UFUNCTION(BlueprintCallable, Category="Lua")
    FLuaRunResult CallLuaFunction(const FString& FunctionName, const TArray<FLuaValue>& Args, int32 InTimeoutMs = 50);

    /** Close and release the sandbox (if private) or clear named reference. */
    UFUNCTION(BlueprintCallable, Category="Lua")
    void ResetSandbox();

private:
    /** Holds a private sandbox when NamedSandbox is not set. */
    UPROPERTY(Transient)
    ULuaSandbox* PrivateSandbox = nullptr;
};
