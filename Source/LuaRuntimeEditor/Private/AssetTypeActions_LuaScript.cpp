#include "AssetTypeActions_LuaScript.h"
#include "LuaScript.h"
#include "AssetToolsModule.h"

UClass* FAssetTypeActions_LuaScript::GetSupportedClass() const
{
    return ULuaScript::StaticClass();
}

uint32 FAssetTypeActions_LuaScript::GetCategories()
{
    // Place under Misc as a default if no custom category is created
    return EAssetTypeCategories::Misc;
}

