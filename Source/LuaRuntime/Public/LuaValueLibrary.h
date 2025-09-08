#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "LuaValue.h"
#include "LuaValueLibrary.generated.h"

USTRUCT(BlueprintType)
struct FLuaKeyValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    FString Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lua")
    FLuaDynValue Value;
};

UCLASS()
class LUARUNTIME_API ULuaValueLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static FLuaDynValue MakeLuaNil();

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static FLuaDynValue MakeLuaBoolean(bool b);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static FLuaDynValue MakeLuaNumber(double n);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static FLuaDynValue MakeLuaString(const FString& s);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static FLuaDynValue MakeLuaArray(const TArray<FLuaDynValue>& Items);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static FLuaDynValue MakeLuaTable(const TArray<FLuaKeyValue>& Pairs);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_IsNil(const FLuaDynValue& V) { return V.Type == ELuaType::Nil; }

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_AsBoolean(const FLuaDynValue& V, bool& bOut, bool bDefault=false);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_AsNumber(const FLuaDynValue& V, double& Out, double Default=0.0);

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_AsString(const FLuaDynValue& V, FString& Out, const FString& Default=TEXT(""));

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_IsArray(const FLuaDynValue& V) { return V.Type == ELuaType::Array; }

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_IsTable(const FLuaDynValue& V) { return V.Type == ELuaType::Table; }

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static int32 LuaValue_ArrayLength(const FLuaDynValue& V) { return V.Type == ELuaType::Array ? V.Array.Num() : 0; }

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_GetArrayItem(const FLuaDynValue& V, int32 Index, FLuaDynValue& Out) { if (V.Type==ELuaType::Array && V.Array.IsValidIndex(Index)) { const TObjectPtr<ULuaValueObject>& ObjPtr = V.Array[Index]; ULuaValueObject* Obj = ObjPtr.Get(); if (Obj) { Out = Obj->Value; return true; } } Out = FLuaDynValue(); return false; }

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static TArray<FString> LuaValue_GetTableKeys(const FLuaDynValue& V) { TArray<FString> Keys; if (V.Type==ELuaType::Table) { V.Table.GetKeys(Keys); } return Keys; }

    UFUNCTION(BlueprintPure, Category="Lua|Values")
    static bool LuaValue_TryGetTableValue(const FLuaDynValue& V, const FString& Key, FLuaDynValue& Out) { if (V.Type==ELuaType::Table) { const TObjectPtr<ULuaValueObject>* PV = V.Table.Find(Key); if (PV) { ULuaValueObject* Obj = PV->Get(); if (Obj) { Out = Obj->Value; return true; } } } Out = FLuaDynValue(); return false; }

    UFUNCTION(BlueprintPure, Category="Lua|Values", meta=(DisplayName="Lua Value → JSON"))
    static FString LuaValue_ToJson(const FLuaDynValue& V);

    UFUNCTION(BlueprintPure, Category="Lua|Values", meta=(DisplayName="JSON → Lua Value"))
    static FLuaDynValue LuaValue_FromJson(const FString& Json);
};
