#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LuaValue.generated.h"

class ULuaValueObject;

UENUM(BlueprintType)
enum class ELuaType : uint8
{
    Nil      UMETA(DisplayName="Nil"),
    Boolean  UMETA(DisplayName="Boolean"),
    Number   UMETA(DisplayName="Number"),
    String   UMETA(DisplayName="String"),
    Array    UMETA(DisplayName="Array"),
    Table    UMETA(DisplayName="Table")
};

USTRUCT(BlueprintType)
struct FLuaDynValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    ELuaType Type = ELuaType::Nil;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="Type==ELuaType::String"))
    FString String;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="Type==ELuaType::Number"))
    double Number = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="Type==ELuaType::Boolean"))
    bool Boolean = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="Type==ELuaType::Array"))
    TArray<TObjectPtr<ULuaValueObject>> Array;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua", meta=(EditCondition="Type==ELuaType::Table"))
    TMap<FString, TObjectPtr<ULuaValueObject>> Table;
};

UCLASS(BlueprintType)
class LUARUNTIME_API ULuaValueObject : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    FLuaDynValue Value;
};
