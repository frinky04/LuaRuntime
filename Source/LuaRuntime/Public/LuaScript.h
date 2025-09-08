#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaScript.generated.h"

/**
 * ULuaScript stores Lua source code as a content asset.
 * Includes simple syntax validation (editor only) and helper accessors.
 */
UCLASS(BlueprintType)
class LUARUNTIME_API ULuaScript : public UObject
{
    GENERATED_BODY()

public:
#if WITH_EDITOR
    // Validate syntax and log errors/warnings in the Output Log.
    UFUNCTION(CallInEditor, Category="Lua")
    void ValidateSyntax();
#endif

    // Get the current source text.
    UFUNCTION(BlueprintPure, Category="Lua")
    const FString& GetSource() const { return Source; }

    // Replace the source text.
    UFUNCTION(BlueprintCallable, Category="Lua")
    void SetSource(const FString& InSource) { Source = InSource; }

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(MultiLine=true))
    FString Source;

    // Optional: if treated as a module, loaders may prefer require/module semantics later.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    bool bTreatAsModule = false;

protected:
#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

