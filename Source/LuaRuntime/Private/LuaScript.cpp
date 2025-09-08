#include "LuaScript.h"
#include "Misc/MessageDialog.h"

// Lua headers for syntax validation
extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#if WITH_EDITOR
void ULuaScript::ValidateSyntax()
{
    FString Error;
    lua_State* L = luaL_newstate();
    if (!L)
    {
        UE_LOG(LogTemp, Error, TEXT("[LuaScript] Failed to create Lua state for validation"));
        return;
    }

    FTCHARToUTF8 CodeUtf8(*Source);
    int loadStatus = luaL_loadbufferx(L, CodeUtf8.Get(), CodeUtf8.Length(), "LuaScript", "t");
    
    if (loadStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        Error = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown syntax error");
        UE_LOG(LogTemp, Error, TEXT("[LuaScript] Syntax error: %s"), *Error);
    }
    else
    {
        UE_LOG(LogTemp, Display, TEXT("[LuaScript] Syntax OK"));
    }

    lua_close(L);
}

void ULuaScript::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    // Auto-validate on source edits
    if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(ULuaScript, Source))
    {
        ValidateSyntax();
    }
}
#endif // WITH_EDITOR

