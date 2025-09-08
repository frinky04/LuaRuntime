#include "LuaValueLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

// Forward declarations of helpers
static TSharedPtr<FJsonValue> ToJson(const FLuaDynValue& V);
static FLuaDynValue FromJson(const TSharedPtr<FJsonValue>& JV);
static void AppendJson(const FLuaDynValue& V, FString& Out);
static FLuaDynValue ParseJson(const FString& In);

FLuaDynValue ULuaValueLibrary::MakeLuaNil()
{
    FLuaDynValue V; V.Type = ELuaType::Nil; return V;
}

FLuaDynValue ULuaValueLibrary::MakeLuaBoolean(bool b)
{
    FLuaDynValue V; V.Type = ELuaType::Boolean; V.Boolean = b; return V;
}

FLuaDynValue ULuaValueLibrary::MakeLuaNumber(double n)
{
    FLuaDynValue V; V.Type = ELuaType::Number; V.Number = n; return V;
}

FLuaDynValue ULuaValueLibrary::MakeLuaString(const FString& s)
{
    FLuaDynValue V; V.Type = ELuaType::String; V.String = s; return V;
}

FLuaDynValue ULuaValueLibrary::MakeLuaArray(const TArray<FLuaDynValue>& Items)
{
    FLuaDynValue V; V.Type = ELuaType::Array; V.Array.Reset();
    for (const FLuaDynValue& Item : Items)
    {
        ULuaValueObject* Obj = NewObject<ULuaValueObject>();
        Obj->Value = Item;
        V.Array.Add(Obj);
    }
    return V;
}

FLuaDynValue ULuaValueLibrary::MakeLuaTable(const TArray<FLuaKeyValue>& Pairs)
{
    FLuaDynValue V; V.Type = ELuaType::Table; V.Table.Empty();
    for (const FLuaKeyValue& P : Pairs)
    {
        ULuaValueObject* Obj = NewObject<ULuaValueObject>();
        Obj->Value = P.Value;
        V.Table.Add(P.Key, Obj);
    }
    return V;
}

bool ULuaValueLibrary::LuaValue_AsBoolean(const FLuaDynValue& V, bool& bOut, bool bDefault)
{
    if (V.Type == ELuaType::Boolean)
    {
        bOut = V.Boolean; return true;
    }
    bOut = bDefault; return false;
}

bool ULuaValueLibrary::LuaValue_AsNumber(const FLuaDynValue& V, double& Out, double Default)
{
    if (V.Type == ELuaType::Number)
    {
        Out = V.Number; return true;
    }
    Out = Default; return false;
}

bool ULuaValueLibrary::LuaValue_AsString(const FLuaDynValue& V, FString& Out, const FString& Default)
{
    if (V.Type == ELuaType::String)
    {
        Out = V.String; return true;
    }
    Out = Default; return false;
}

FString ULuaValueLibrary::LuaValue_ToJson(const FLuaDynValue& V)
{
    FString S; AppendJson(V, S); return S;
}

FLuaDynValue ULuaValueLibrary::LuaValue_FromJson(const FString& Json)
{
    return ParseJson(Json);
}

static TSharedPtr<FJsonValue> ToJson(const FLuaDynValue& V)
{
    switch (V.Type)
    {
    case ELuaType::Nil: return MakeShared<FJsonValueNull>();
    case ELuaType::Boolean: return MakeShared<FJsonValueBoolean>(V.Boolean);
    case ELuaType::Number: return MakeShared<FJsonValueNumber>(V.Number);
    case ELuaType::String: return MakeShared<FJsonValueString>(V.String);
    case ELuaType::Array:
    {
        TArray<TSharedPtr<FJsonValue>> Arr;
        Arr.Reserve(V.Array.Num());
        for (const TObjectPtr<ULuaValueObject>& EPtr : V.Array) { ULuaValueObject* E = EPtr.Get(); Arr.Add(E ? ToJson(E->Value) : MakeShared<FJsonValueNull>()); }
        return MakeShared<FJsonValueArray>(MoveTemp(Arr));
    }
    case ELuaType::Table:
    {
        TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
        for (const auto& Pair : V.Table)
        {
            ULuaValueObject* Child = Pair.Value.Get();
            Obj->SetField(Pair.Key, Child ? ToJson(Child->Value) : MakeShared<FJsonValueNull>());
        }
        return MakeShared<FJsonValueObject>(Obj);
    }
    }
    return MakeShared<FJsonValueNull>();
}

static FLuaDynValue FromJson(const TSharedPtr<FJsonValue>& JV)
{
    FLuaDynValue V; V.Type = ELuaType::Nil;
    if (!JV.IsValid()) return V;
    switch (JV->Type)
    {
    case EJson::None: V.Type = ELuaType::Nil; break;
    case EJson::Null: V.Type = ELuaType::Nil; break;
    case EJson::Boolean: V.Type = ELuaType::Boolean; V.Boolean = JV->AsBool(); break;
    case EJson::Number: V.Type = ELuaType::Number; V.Number = JV->AsNumber(); break;
    case EJson::String: V.Type = ELuaType::String; V.String = JV->AsString(); break;
    case EJson::Array:
    {
        V.Type = ELuaType::Array; const TArray<TSharedPtr<FJsonValue>>& Arr = JV->AsArray();
        for (const auto& E : Arr) { ULuaValueObject* Elem = NewObject<ULuaValueObject>(); Elem->Value = FromJson(E); V.Array.Add(Elem); }
        break;
    }
    case EJson::Object:
    {
        V.Type = ELuaType::Table; const TSharedPtr<FJsonObject>& Obj = JV->AsObject();
        for (const auto& Kvp : Obj->Values)
        {
            ULuaValueObject* Child = NewObject<ULuaValueObject>();
            Child->Value = FromJson(Kvp.Value);
            V.Table.Add(Kvp.Key, Child);
        }
        break;
    }
    }
    return V;
}


// Extra QoL helpers
static void AppendJson(const FLuaDynValue& V, FString& Out)
{
    TSharedPtr<FJsonValue> Root = ToJson(V);
    FString Str;
    auto Writer = TJsonWriterFactory<>::Create(&Str);
    FJsonSerializer::Serialize(Root.ToSharedRef(), TEXT(""), Writer);
    Out = MoveTemp(Str);
}

static FLuaDynValue ParseJson(const FString& In)
{
    TSharedPtr<FJsonValue> Root;
    auto Reader = TJsonReaderFactory<>::Create(In);
    if (FJsonSerializer::Deserialize(Reader, Root) && Root.IsValid())
    {
        return FromJson(Root);
    }
    FLuaDynValue V; V.Type = ELuaType::Nil; return V;
}
