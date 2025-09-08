#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LuaRuntimeSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Lua Runtime"))
class LUARUNTIME_API ULuaRuntimeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    ULuaRuntimeSettings();

    // Place settings under Project Settings -> Plugins -> LuaRuntime
    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FName GetSectionName() const override { return TEXT("LuaRuntime"); }

public: // Printing
    /** Show Lua print() messages on screen (uses GEngine->AddOnScreenDebugMessage). */
    UPROPERTY(EditAnywhere, Config, Category="Printing")
    bool bOnScreenPrintEnabled;

    /** Duration (seconds) for on-screen print messages. */
    UPROPERTY(EditAnywhere, Config, Category="Printing", meta=(ClampMin="0.1", UIMin="0.1"))
    float OnScreenPrintDuration;

    /** Text color for on-screen print messages. */
    UPROPERTY(EditAnywhere, Config, Category="Printing")
    FLinearColor OnScreenPrintColor;

    /** Allow on-screen print in Shipping builds (off by default). */
    UPROPERTY(EditAnywhere, Config, Category="Printing")
    bool bAllowOnScreenPrintInShipping;

public: // Execution Defaults
    /** Default memory budget (KB) for new sandboxes. */
    UPROPERTY(EditAnywhere, Config, Category="Execution", meta=(ClampMin="64", UIMin="64"))
    int32 DefaultMemoryLimitKB;

    /** Default timeout (ms) for script execution. */
    UPROPERTY(EditAnywhere, Config, Category="Execution", meta=(ClampMin="1", UIMin="1"))
    int32 DefaultTimeoutMs;

    /** Default hook interval (instructions) for timeout checks. */
    UPROPERTY(EditAnywhere, Config, Category="Execution", meta=(ClampMin="1", UIMin="1"))
    int32 DefaultHookInterval;
};

