#include "LuaComponent.h"
#include "LuaRuntimeSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LuaScript.h"

ULuaComponent::ULuaComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void ULuaComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bAutoRunOnBeginPlay)
    {
        ExecuteConfiguredScript();
    }
}

void ULuaComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetSandbox();
    Super::EndPlay(EndPlayReason);
}

ULuaSandbox* ULuaComponent::EnsureSandbox()
{
    if (!GetWorld())
    {
        return nullptr;
    }

    if (NamedSandbox.IsNone())
    {
        if (!PrivateSandbox)
        {
            if (UGameInstance* GI = GetWorld()->GetGameInstance())
            {
                if (ULuaRuntimeSubsystem* Subsys = GI->GetSubsystem<ULuaRuntimeSubsystem>())
                {
                    PrivateSandbox = Subsys->CreateSandbox(MemoryLimitKB);
                }
            }
        }
        return PrivateSandbox;
    }
    else
    {
        if (UGameInstance* GI = GetWorld()->GetGameInstance())
        {
            if (ULuaRuntimeSubsystem* Subsys = GI->GetSubsystem<ULuaRuntimeSubsystem>())
            {
                ULuaSandbox* Box = Subsys->GetNamedSandbox(NamedSandbox);
                if (!Box)
                {
                    Box = Subsys->CreateNamedSandbox(NamedSandbox, MemoryLimitKB);
                }
                return Box;
            }
        }
    }

    return nullptr;
}

FLuaRunResult ULuaComponent::ExecuteConfiguredScript()
{
    FLuaRunResult Result; Result.bSuccess = false;

    ULuaSandbox* Box = EnsureSandbox();
    if (!Box)
    {
        Result.Error = TEXT("No sandbox available");
        return Result;
    }

    if (bUseFile)
    {
        return Box->RunFile(ScriptFilePath, TimeoutMs, HookInterval);
    }
    else if (bUseAsset && ScriptAsset)
    {
        return Box->RunString(ScriptAsset->GetSource(), TimeoutMs, HookInterval);
    }
    else
    {
        return Box->RunString(ScriptCode, TimeoutMs, HookInterval);
    }
}

FLuaRunResult ULuaComponent::CallLuaFunction(const FString& FunctionName, const TArray<FLuaValue>& Args, int32 InTimeoutMs)
{
    FLuaRunResult Result; Result.bSuccess = false;
    ULuaSandbox* Box = EnsureSandbox();
    if (!Box)
    {
        Result.Error = TEXT("No sandbox available");
        return Result;
    }
    return Box->CallFunction(FunctionName, Args, InTimeoutMs);
}

void ULuaComponent::ResetSandbox()
{
    if (!GetWorld())
    {
        PrivateSandbox = nullptr;
        return;
    }

    if (NamedSandbox.IsNone())
    {
        if (PrivateSandbox)
        {
            PrivateSandbox->Close();
            PrivateSandbox = nullptr;
        }
    }
    else
    {
        if (UGameInstance* GI = GetWorld()->GetGameInstance())
        {
            if (ULuaRuntimeSubsystem* Subsys = GI->GetSubsystem<ULuaRuntimeSubsystem>())
            {
                Subsys->RemoveNamedSandbox(NamedSandbox);
            }
        }
    }
}
