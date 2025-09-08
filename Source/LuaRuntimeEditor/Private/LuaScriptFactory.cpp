#include "LuaScriptFactory.h"
#include "LuaScript.h"

ULuaScriptFactory::ULuaScriptFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = ULuaScript::StaticClass();
}

UObject* ULuaScriptFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<ULuaScript>(InParent, Class, Name, Flags | RF_Transactional);
}

