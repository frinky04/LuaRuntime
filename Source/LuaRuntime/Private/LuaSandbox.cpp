#include "LuaSandbox.h"
#include "LuaRuntime.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Engine/Engine.h" // GEngine->AddOnScreenDebugMessage
#include "LuaRuntimeSettings.h"
#include <cstdlib>

// Lua headers (vendored under Private/ThirdParty/lua_slim/src)
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

namespace {

struct FAllocatorState
{
    int64 LimitBytes = 0;
    int64 UsedBytes = 0;
};

static void* LuaCountingAllocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
    FAllocatorState* State = reinterpret_cast<FAllocatorState*>(ud);

    // Adjust usage by the delta
    int64 Delta = (int64)nsize - (int64)osize;
    int64 NewUsed = State->UsedBytes + Delta;

    if (nsize != 0 && State->LimitBytes > 0 && NewUsed > State->LimitBytes)
    {
        // Allocation would exceed the cap
        return nullptr;
    }

    // Perform the reallocation
    void* NewPtr = nullptr;
    if (nsize == 0)
    {
        // Free
        free(ptr);
        NewPtr = nullptr;
    }
    else if (ptr == nullptr)
    {
        NewPtr = malloc(nsize);
    }
    else
    {
        NewPtr = realloc(ptr, nsize);
    }

    if (NewPtr || nsize == 0)
    {
        State->UsedBytes = NewUsed;
    }
    return NewPtr;
}

// Wall-clock timeout via instruction hook
struct FHookState
{
    double StartTimeSec = 0.0;
    int32 TimeoutMs = 0;
};

static void HookTimeout(lua_State* L)
{
    FHookState* HS = *reinterpret_cast<FHookState**>(lua_getextraspace(L));
    if (!HS || HS->TimeoutMs <= 0) return;
    const double ElapsedMs = (FPlatformTime::Seconds() - HS->StartTimeSec) * 1000.0;
    if (ElapsedMs > (double)HS->TimeoutMs)
    {
        luaL_error(L, "execution timed out");
    }
}

static void LuaHook(lua_State* L, lua_Debug* /*ar*/)
{
    HookTimeout(L);
}

// Recursively convert a Lua value at a given index to FLuaDynValue.
// Performs deep copy for tables; detects array-like tables (1..N integer keys only).
static void ConvertLuaToDynValue(lua_State* L, int Index, FLuaDynValue& Out, ULuaSandbox* Owner, int Depth = 0, int MaxDepth = 32, TSet<const void*>* InVisited = nullptr)
{
    const int absIndex = lua_absindex(L, Index);

    const int type = lua_type(L, absIndex);
    switch (type)
    {
    case LUA_TNIL:
        Out.Type = ELuaType::Nil; return;
    case LUA_TBOOLEAN:
        Out.Type = ELuaType::Boolean; Out.Boolean = lua_toboolean(L, absIndex) != 0; return;
    case LUA_TNUMBER:
        Out.Type = ELuaType::Number; Out.Number = lua_tonumber(L, absIndex); return;
    case LUA_TSTRING:
    {
        size_t len = 0; const char* s = lua_tolstring(L, absIndex, &len);
        Out.Type = ELuaType::String; Out.String = FString(len, UTF8_TO_TCHAR(s)); return;
    }
    case LUA_TTABLE:
    {
        Out.Type = ELuaType::Table;
        if (Depth >= MaxDepth)
        {
            // Depth cap; do not descend further
            return;
        }
        TSet<const void*> LocalVisited;
        TSet<const void*>* Visited = InVisited ? InVisited : &LocalVisited;
        const void* Ptr = lua_topointer(L, absIndex);
        if (Visited->Contains(Ptr))
        {
            // Cycle detected; stop here
            return;
        }
        Visited->Add(Ptr);

        const lua_Integer rawLen = (lua_Integer)lua_rawlen(L, absIndex);
        bool bArrayCandidate = true;
        int32 Count = 0;

        lua_pushnil(L);
        while (lua_next(L, absIndex) != 0)
        {
            // stack: ... key value
            if (lua_type(L, -2) == LUA_TNUMBER)
            {
                lua_Number n = lua_tonumber(L, -2);
                lua_Integer i;
                if (modf(n, &n) != 0.0) // non-integer
                {
                    bArrayCandidate = false;
                }
                else
                {
                    i = (lua_Integer)lua_tointeger(L, -2);
                    if (i < 1 || i > rawLen)
                    {
                        bArrayCandidate = false;
                    }
                }
            }
            else
            {
                bArrayCandidate = false;
            }
            Count++;
            lua_pop(L, 1); // pop value, keep key for next lua_next
        }

        if (bArrayCandidate && Count == rawLen)
        {
            Out.Type = ELuaType::Array;
            Out.Array.Reserve((int32)rawLen);
            for (lua_Integer i = 1; i <= rawLen; ++i)
            {
                lua_geti(L, absIndex, i);
                ULuaValueObject* ElemObj = NewObject<ULuaValueObject>(Owner);
                ConvertLuaToDynValue(L, -1, ElemObj->Value, Owner, Depth + 1, MaxDepth, Visited);
                lua_pop(L, 1);
                Out.Array.Add(ElemObj);
            }
        }
        else
        {
            Out.Type = ELuaType::Table;
            Out.Table.Empty();
            lua_pushnil(L);
            while (lua_next(L, absIndex) != 0)
            {
                // key at -2, value at -1
                FString KeyStr;
                int kt = lua_type(L, -2);
                switch (kt)
                {
                case LUA_TSTRING:
                {
                    size_t klen = 0; const char* ks = lua_tolstring(L, -2, &klen);
                    KeyStr = FString(klen, UTF8_TO_TCHAR(ks));
                    break;
                }
                case LUA_TNUMBER:
                {
                    lua_Number kn = lua_tonumber(L, -2);
                    KeyStr = FString::SanitizeFloat(kn);
                    break;
                }
                case LUA_TBOOLEAN:
                    KeyStr = lua_toboolean(L, -2) ? TEXT("true") : TEXT("false");
                    break;
                default:
                {
                    // Fallback to tostring for complex keys
                    size_t klen = 0; const char* ks = luaL_tolstring(L, -2, &klen);
                    KeyStr = FString(klen, UTF8_TO_TCHAR(ks));
                    lua_pop(L, 1); // pop tostring result
                    break;
                }
                }

                ULuaValueObject* ChildObj = NewObject<ULuaValueObject>(Owner);
                ConvertLuaToDynValue(L, -1, ChildObj->Value, Owner, Depth + 1, MaxDepth, Visited);
                Out.Table.Add(MoveTemp(KeyStr), ChildObj);
                lua_pop(L, 1); // pop value
            }
        }

        Visited->Remove(Ptr);
        return;
    }
    default:
        Out.Type = ELuaType::Nil; return;
    }
}

static int LuaPrint(lua_State* L)
{
    int nargs = lua_gettop(L);
    FString Out;
    Out.Reserve(128);
    for (int i = 1; i <= nargs; ++i)
    {
        size_t len = 0;
        const char* s = luaL_tolstring(L, i, &len);
        if (s)
        {
            if (i > 1) Out += TEXT("\t");
            Out += FString(len, UTF8_TO_TCHAR(s));
        }
        lua_pop(L, 1); // pop tostring result
    }
    UE_LOG(LogLuaRuntime, Log, TEXT("[lua] %s"), *Out);

    if (GEngine)
    {
        const ULuaRuntimeSettings* Settings = GetDefault<ULuaRuntimeSettings>();
        bool bAllowOnScreen = Settings && Settings->bOnScreenPrintEnabled;
#if UE_BUILD_SHIPPING
        bAllowOnScreen = bAllowOnScreen && Settings && Settings->bAllowOnScreenPrintInShipping;
#endif
        if (bAllowOnScreen)
        {
            const float Duration = Settings ? Settings->OnScreenPrintDuration : 4.0f;
            const FColor Color = Settings ? Settings->OnScreenPrintColor.ToFColor(true) : FColor::Green;
            GEngine->AddOnScreenDebugMessage(/*Key*/-1, Duration, Color, FString::Printf(TEXT("[lua] %s"), *Out));
        }
    }
    return 0;
}

// Helper: set global string from UTF-16
static void PushFString(lua_State* L, const FString& Str)
{
    FTCHARToUTF8 Convert(*Str);
    lua_pushlstring(L, Convert.Get(), Convert.Length());
}

}

ULuaSandbox::ULuaSandbox()
{
}

void ULuaSandbox::BeginDestroy()
{
    Close();
    Super::BeginDestroy();
}

void* ULuaSandbox::CreateState(int32 MemoryLimitKB)
{
    check(L == nullptr);
    AllocLimitBytes = (int64)MemoryLimitKB * 1024;

    // Hook state pointer stored in extraspace
    // Allocator state lives separately
    FAllocatorState* AllocState = new FAllocatorState();
    AllocState->LimitBytes = AllocLimitBytes;
    AllocState->UsedBytes = 0;

    lua_State* NewL = lua_newstate(&LuaCountingAllocator, AllocState);
    if (!NewL)
    {
        delete AllocState;
        return nullptr;
    }

    // Panic handler: turn panic into UE log and close
    lua_atpanic(NewL, [](lua_State* LL) -> int {
        const char* msg = lua_tostring(LL, -1);
        UE_LOG(LogLuaRuntime, Error, TEXT("Lua panic: %s"), msg ? UTF8_TO_TCHAR(msg) : TEXT("<null>"));
        return 0;
    });

    // Initialize hook state pointer in extraspace
    FHookState* HS = new FHookState();
    HS->StartTimeSec = 0.0;
    HS->TimeoutMs = 0;
    *reinterpret_cast<FHookState**>(lua_getextraspace(NewL)) = HS;

    return NewL;
}

void ULuaSandbox::OpenSafeLibs()
{
    check(L);
    // Open only safe libs
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);  lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1); lua_pop(L, 1);

    RemoveUnsafeBaseFuncs();
    InstallPrint();
}

void ULuaSandbox::RemoveUnsafeBaseFuncs()
{
    // Remove base functions that touch the filesystem or load binary chunks
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "load");

    // Also remove string.dump (generates binary chunks)
    lua_getglobal(L, "string");
    if (lua_istable(L, -1))
    {
        lua_pushnil(L);
        lua_setfield(L, -2, "dump");
    }
    lua_pop(L, 1);
}

void ULuaSandbox::InstallPrint()
{
    lua_pushcfunction(L, &LuaPrint);
    lua_setglobal(L, "print");
}

void ULuaSandbox::Initialize(int32 MemoryLimitKB)
{
    if (L)
    {
        Close();
    }
    L = static_cast<lua_State*>(CreateState(MemoryLimitKB));
    if (L)
    {
        OpenSafeLibs();
    }
}

void ULuaSandbox::Close()
{
    if (L)
    {
        // Free hook state pointer first
        FHookState* HS = *reinterpret_cast<FHookState**>(lua_getextraspace(L));
        delete HS;
        *reinterpret_cast<FHookState**>(lua_getextraspace(L)) = nullptr;

        void* UD = nullptr;
        lua_Alloc AllocFunc = lua_getallocf(L, &UD);
        (void)AllocFunc; // unused
        lua_close(L);
        L = nullptr;
        delete reinterpret_cast<FAllocatorState*>(UD);
    }
}

FLuaRunResult ULuaSandbox::RunString(const FString& Code, int32 TimeoutMs, int32 HookInterval)
{
    FLuaRunResult Result;
    if (!L)
    {
        Result.bSuccess = false;
        Result.Error = TEXT("Lua state is not initialized");
        return Result;
    }

    // Load chunk with text-only mode
    FTCHARToUTF8 CodeUtf8(*Code);
    int loadStatus = luaL_loadbufferx(L, CodeUtf8.Get(), CodeUtf8.Length(), "chunk", "t");
    if (loadStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        Result.bSuccess = false;
        Result.Error = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown load error");
        lua_pop(L, 1);
        return Result;
    }

    // Set timeout hook
    FHookState* HS = *reinterpret_cast<FHookState**>(lua_getextraspace(L));
    HS->StartTimeSec = FPlatformTime::Seconds();
    HS->TimeoutMs = TimeoutMs;
    lua_sethook(L, &LuaHook, LUA_MASKCOUNT, FMath::Max(1, HookInterval));

    // pcall with 0 args, and capture return values
    int callStatus = lua_pcall(L, 0, LUA_MULTRET, 0);

    // Clear hook
    lua_sethook(L, nullptr, 0, 0);

    if (callStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        Result.bSuccess = false;
        Result.Error = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown runtime error");
        lua_pop(L, 1);
        return Result;
    }

    // Capture the first return value if any
    if (lua_gettop(L) > 0)
    {
        size_t len = 0;
        const char* s = lua_tolstring(L, -1, &len);
        if (s)
        {
            Result.ReturnValue = FString(len, UTF8_TO_TCHAR(s));
        }
        lua_pop(L, lua_gettop(L)); // Clear the stack
    }

    Result.bSuccess = true;
    return Result;
}

void ULuaSandbox::SetGlobalNumber(const FName Name, double Value)
{
    if (!L) return;
    lua_pushnumber(L, Value);
    lua_setglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
}

void ULuaSandbox::SetGlobalString(const FName Name, const FString& Value)
{
    if (!L) return;
    PushFString(L, Value);
    lua_setglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
}

bool ULuaSandbox::GetGlobalNumber(const FName Name, double& OutValue) const
{
    if (!L) return false;
    lua_getglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
    if (lua_isnumber(L, -1))
    {
        OutValue = lua_tonumber(L, -1);
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

bool ULuaSandbox::GetGlobalString(const FName Name, FString& OutValue) const
{
    if (!L) return false;
    lua_getglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
    if (lua_isstring(L, -1))
    {
        size_t len = 0;
        const char* s = lua_tolstring(L, -1, &len);
        OutValue = FString(len, UTF8_TO_TCHAR(s));
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

void ULuaSandbox::SetGlobalBool(const FName Name, bool Value)
{
    if (!L) return;
    lua_pushboolean(L, Value ? 1 : 0);
    lua_setglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
}

bool ULuaSandbox::GetGlobalBool(const FName Name, bool& OutValue) const
{
    if (!L) return false;
    lua_getglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
    if (lua_isboolean(L, -1))
    {
        OutValue = lua_toboolean(L, -1) != 0;
        lua_pop(L, 1);
        return true;
    }
    lua_pop(L, 1);
    return false;
}

void ULuaSandbox::SetGlobalDyn(const FName Name, const FLuaDynValue& Value)
{
    if (!L) return;
    PushLuaDynValue(Value);
    lua_setglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
}

bool ULuaSandbox::GetGlobalDyn(const FName Name, FLuaDynValue& OutValue) const
{
    if (!L) return false;
    lua_getglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
    if (lua_isnil(L, -1))
    {
        lua_pop(L, 1);
        OutValue.Type = ELuaType::Nil;
        return false;
    }
    OutValue = PopLuaDynValue();
    return OutValue.Type != ELuaType::Nil;
}

FLuaRunResult ULuaSandbox::CallFunction(const FString& FunctionName, const TArray<FLuaValue>& Args, int32 TimeoutMs)
{
    FLuaRunResult Result;
    if (!L)
    {
        Result.bSuccess = false;
        Result.Error = TEXT("Lua state not initialized");
        return Result;
    }

    lua_getglobal(L, TCHAR_TO_UTF8(*FunctionName));
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        Result.bSuccess = false;
        Result.Error = FString::Printf(TEXT("'%s' is not a function"), *FunctionName);
        return Result;
    }

    for (const FLuaValue& Arg : Args)
    {
        PushLuaValue(Arg);
    }

    // Set timeout hook
    FHookState* HS = *reinterpret_cast<FHookState**>(lua_getextraspace(L));
    HS->StartTimeSec = FPlatformTime::Seconds();
    HS->TimeoutMs = TimeoutMs;
    lua_sethook(L, &LuaHook, LUA_MASKCOUNT, FMath::Max(1, 1000));

    int callStatus = lua_pcall(L, Args.Num(), 1, 0);
    
    // Clear hook
    lua_sethook(L, nullptr, 0, 0);

    if (callStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        Result.bSuccess = false;
        Result.Error = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown runtime error");
        lua_pop(L, 1);
        return Result;
    }

    if (!lua_isnil(L, -1))
    {
        size_t len = 0;
        const char* s = lua_tolstring(L, -1, &len);
        if (s)
        {
            Result.ReturnValue = FString(len, UTF8_TO_TCHAR(s));
        }
    }
    lua_pop(L, 1);

    Result.bSuccess = true;
    return Result;
}

FLuaRunResult ULuaSandbox::CallFunctionDyn(const FString& FunctionName, const TArray<FLuaDynValue>& Args, int32 TimeoutMs)
{
    FLuaRunResult Result;
    if (!L)
    {
        Result.bSuccess = false;
        Result.Error = TEXT("Lua state not initialized");
        return Result;
    }

    lua_getglobal(L, TCHAR_TO_UTF8(*FunctionName));
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 1);
        Result.bSuccess = false;
        Result.Error = FString::Printf(TEXT("'%s' is not a function"), *FunctionName);
        return Result;
    }

    for (const FLuaDynValue& Arg : Args)
    {
        PushLuaDynValue(Arg);
    }

    // Set timeout hook
    FHookState* HS = *reinterpret_cast<FHookState**>(lua_getextraspace(L));
    HS->StartTimeSec = FPlatformTime::Seconds();
    HS->TimeoutMs = TimeoutMs;
    lua_sethook(L, &LuaHook, LUA_MASKCOUNT, FMath::Max(1, 1000));

    int callStatus = lua_pcall(L, Args.Num(), 1, 0);
    
    // Clear hook
    lua_sethook(L, nullptr, 0, 0);

    if (callStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        Result.bSuccess = false;
        Result.Error = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown runtime error");
        lua_pop(L, 1);
        return Result;
    }

    if (!lua_isnil(L, -1))
    {
        size_t len = 0;
        const char* s = lua_tolstring(L, -1, &len);
        if (s)
        {
            Result.ReturnValue = FString(len, UTF8_TO_TCHAR(s));
        }
    }
    lua_pop(L, 1);

    Result.bSuccess = true;
    return Result;
}

bool ULuaSandbox::HasGlobal(const FName Name) const
{
    if (!L) return false;
    lua_getglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
    bool exists = !lua_isnil(L, -1);
    lua_pop(L, 1);
    return exists;
}

void ULuaSandbox::ClearGlobal(const FName Name)
{
    if (!L) return;
    lua_pushnil(L);
    lua_setglobal(L, TCHAR_TO_UTF8(*Name.ToString()));
}

TArray<FString> ULuaSandbox::GetGlobalNames() const
{
    TArray<FString> Names;
    if (!L) return Names;

    lua_pushglobaltable(L);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0)
    {
        if (lua_type(L, -2) == LUA_TSTRING)
        {
            const char* key = lua_tostring(L, -2);
            Names.Add(UTF8_TO_TCHAR(key));
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    return Names;
}

bool ULuaSandbox::SetTableValue(const FString& TablePath, const FString& Key, const FLuaValue& Value)
{
    if (!L) return false;

    if (!GetTableByPath(TablePath))
    {
        return false;
    }

    FTCHARToUTF8 Convert(*Key);
    lua_pushlstring(L, Convert.Get(), Convert.Length());
    PushLuaValue(Value);
    lua_settable(L, -3);
    lua_pop(L, 1);

    return true;
}

bool ULuaSandbox::SetTableValueDyn(const FString& TablePath, const FString& Key, const FLuaDynValue& Value)
{
    if (!L) return false;

    if (!GetTableByPath(TablePath))
    {
        return false;
    }

    FTCHARToUTF8 Convert(*Key);
    lua_pushlstring(L, Convert.Get(), Convert.Length());
    PushLuaDynValue(Value);
    lua_settable(L, -3);
    lua_pop(L, 1);

    return true;
}

bool ULuaSandbox::GetTableValue(const FString& TablePath, const FString& Key, FLuaValue& OutValue) const
{
    if (!L) return false;

    if (!GetTableByPath(TablePath))
    {
        return false;
    }

    FTCHARToUTF8 Convert(*Key);
    lua_pushlstring(L, Convert.Get(), Convert.Length());
    lua_gettable(L, -2);
    OutValue = PopLuaValue();
    lua_pop(L, 1);

    return !OutValue.bIsNil;
}

bool ULuaSandbox::GetTableValueDyn(const FString& TablePath, const FString& Key, FLuaDynValue& OutValue) const
{
    if (!L) return false;

    if (!GetTableByPath(TablePath))
    {
        return false;
    }

    FTCHARToUTF8 Convert(*Key);
    lua_pushlstring(L, Convert.Get(), Convert.Length());
    lua_gettable(L, -2);
    OutValue = PopLuaDynValue();
    lua_pop(L, 1);

    return OutValue.Type != ELuaType::Nil;
}

FLuaRunResult ULuaSandbox::RunFile(const FString& FilePath, int32 TimeoutMs, int32 HookInterval)
{
    FLuaRunResult Result;
    
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *FilePath))
    {
        Result.bSuccess = false;
        Result.Error = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
        return Result;
    }

    return RunString(Content, TimeoutMs, HookInterval);
}

bool ULuaSandbox::RunStringDyn(const FString& Code, int32 TimeoutMs, int32 HookInterval, FLuaDynValue& OutValue, FString& OutError)
{
    if (!L)
    {
        OutError = TEXT("Lua state is not initialized");
        return false;
    }
    FTCHARToUTF8 CodeUtf8(*Code);
    int loadStatus = luaL_loadbufferx(L, CodeUtf8.Get(), CodeUtf8.Length(), "chunk", "t");
    if (loadStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        OutError = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown load error");
        lua_pop(L, 1);
        return false;
    }

    FHookState* HS = *reinterpret_cast<FHookState**>(lua_getextraspace(L));
    HS->StartTimeSec = FPlatformTime::Seconds();
    HS->TimeoutMs = TimeoutMs;
    lua_sethook(L, &LuaHook, LUA_MASKCOUNT, FMath::Max(1, HookInterval));

    int callStatus = lua_pcall(L, 0, 1, 0);
    
    lua_sethook(L, nullptr, 0, 0);

    if (callStatus != LUA_OK)
    {
        const char* err = lua_tostring(L, -1);
        OutError = err ? UTF8_TO_TCHAR(err) : TEXT("Unknown runtime error");
        lua_pop(L, 1);
        return false;
    }

    OutValue = PopLuaDynValue();
    return true;
}

bool ULuaSandbox::RunFileDyn(const FString& FilePath, int32 TimeoutMs, int32 HookInterval, FLuaDynValue& OutValue, FString& OutError)
{
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *FilePath))
    {
        OutError = FString::Printf(TEXT("Failed to read file: %s"), *FilePath);
        return false;
    }
    return RunStringDyn(Content, TimeoutMs, HookInterval, OutValue, OutError);
}

void ULuaSandbox::RegisterCallback(const FString& CallbackName)
{
    if (!L) return;

    auto CallbackFunc = [](lua_State* LuaState) -> int
    {
        ULuaSandbox* Sandbox = nullptr;
        lua_getallocf(LuaState, reinterpret_cast<void**>(&Sandbox));
        
        if (!Sandbox) return 0;

        lua_getglobal(LuaState, "__callback_name");
        const char* cbName = lua_tostring(LuaState, -1);
        lua_pop(LuaState, 1);

        int NumArgs = lua_gettop(LuaState);
        TArray<FLuaValue> Args;
        for (int i = 1; i <= NumArgs; i++)
        {
            lua_pushvalue(LuaState, i);
            Args.Add(Sandbox->PopLuaValue());
        }

        if (cbName)
        {
            Sandbox->OnLuaCallback.Broadcast(UTF8_TO_TCHAR(cbName), Args);
        }

        return 0;
    };

    lua_pushstring(L, TCHAR_TO_UTF8(*CallbackName));
    lua_setglobal(L, "__callback_name");
    
    lua_pushcfunction(L, CallbackFunc);
    lua_setglobal(L, TCHAR_TO_UTF8(*CallbackName));
}

int64 ULuaSandbox::GetMemoryUsage() const
{
    if (!L) return 0;
    
    FAllocatorState* AllocState = reinterpret_cast<FAllocatorState*>(lua_getextraspace(L));
    return AllocState ? AllocState->UsedBytes : 0;
}

void ULuaSandbox::SetMemoryLimit(int32 NewLimitKB)
{
    if (!L) return;
    
    AllocLimitBytes = (int64)NewLimitKB * 1024;
    FAllocatorState* AllocState = reinterpret_cast<FAllocatorState*>(lua_getextraspace(L));
    if (AllocState)
    {
        AllocState->LimitBytes = AllocLimitBytes;
    }
}

FLuaRunResult ULuaSandbox::EvaluateExpression(const FString& Expression, int32 TimeoutMs)
{
    FString EvalCode = FString::Printf(TEXT("return %s"), *Expression);
    FLuaRunResult Result = RunString(EvalCode, TimeoutMs, 1000);
    
    if (Result.bSuccess && L)
    {
        if (lua_gettop(L) > 0)
        {
            size_t len = 0;
            const char* s = lua_tolstring(L, -1, &len);
            if (s)
            {
                Result.ReturnValue = FString(len, UTF8_TO_TCHAR(s));
            }
            lua_pop(L, 1);
        }
    }
    
    return Result;
}

bool ULuaSandbox::EvaluateExpressionDyn(const FString& Expression, int32 TimeoutMs, FLuaDynValue& OutValue, FString& OutError)
{
    FString EvalCode = FString::Printf(TEXT("return %s"), *Expression);
    return RunStringDyn(EvalCode, TimeoutMs, 1000, OutValue, OutError);
}

void ULuaSandbox::PushLuaValue(const FLuaValue& Value)
{
    if (!L) return;

    if (Value.bIsNil)
    {
        lua_pushnil(L);
    }
    else if (!Value.StringValue.IsEmpty())
    {
        FTCHARToUTF8 Convert(*Value.StringValue);
        lua_pushlstring(L, Convert.Get(), Convert.Length());
    }
    else if (Value.BoolValue)
    {
        lua_pushboolean(L, 1);
    }
    else
    {
        lua_pushnumber(L, Value.NumberValue);
    }
}

FLuaValue ULuaSandbox::PopLuaValue() const
{
    FLuaValue Value;
    if (!L) return Value;

    int type = lua_type(L, -1);
    switch (type)
    {
    case LUA_TNIL:
        Value.bIsNil = true;
        break;
    case LUA_TBOOLEAN:
        Value.BoolValue = lua_toboolean(L, -1) != 0;
        Value.bIsNil = false;
        break;
    case LUA_TNUMBER:
        Value.NumberValue = lua_tonumber(L, -1);
        Value.bIsNil = false;
        break;
    case LUA_TSTRING:
        {
            size_t len = 0;
            const char* s = lua_tolstring(L, -1, &len);
            Value.StringValue = FString(len, UTF8_TO_TCHAR(s));
            Value.bIsNil = false;
        }
        break;
    default:
        Value.bIsNil = true;
        break;
    }

    lua_pop(L, 1);
    return Value;
}

void ULuaSandbox::PushLuaDynValue(const FLuaDynValue& Value)
{
    if (!L) return;
    switch (Value.Type)
    {
    case ELuaType::Nil:
        lua_pushnil(L);
        break;
    case ELuaType::Boolean:
        lua_pushboolean(L, Value.Boolean ? 1 : 0);
        break;
    case ELuaType::Number:
        lua_pushnumber(L, Value.Number);
        break;
    case ELuaType::String:
    {
        FTCHARToUTF8 Convert(*Value.String);
        lua_pushlstring(L, Convert.Get(), Convert.Length());
        break;
    }
    case ELuaType::Array:
    {
        lua_createtable(L, Value.Array.Num(), 0);
        int index = 1;
        for (const TObjectPtr<ULuaValueObject>& ElemObjPtr : Value.Array)
        {
            ULuaValueObject* ElemObj = ElemObjPtr.Get();
            if (ElemObj)
            {
                PushLuaDynValue(ElemObj->Value);
            }
            else
            {
                lua_pushnil(L);
            }
            lua_seti(L, -2, index++);
        }
        break;
    }
    case ELuaType::Table:
    {
        lua_createtable(L, 0, Value.Table.Num());
        for (const auto& Pair : Value.Table)
        {
            FTCHARToUTF8 KeyUtf8(*Pair.Key);
            lua_pushlstring(L, KeyUtf8.Get(), KeyUtf8.Length());
            ULuaValueObject* Child = Pair.Value.Get();
            if (Child)
            {
                PushLuaDynValue(Child->Value);
            }
            else
            {
                lua_pushnil(L);
            }
            lua_settable(L, -3);
        }
        break;
    }
    }
}

FLuaDynValue ULuaSandbox::PopLuaDynValue() const
{
    FLuaDynValue V;
    if (!L) return V;
    ConvertLuaToDynValue(L, -1, V, const_cast<ULuaSandbox*>(this));
    lua_pop(L, 1);
    return V;
}

bool ULuaSandbox::GetTableByPath(const FString& TablePath) const
{
    if (!L) return false;

    TArray<FString> Parts;
    TablePath.ParseIntoArray(Parts, TEXT("."), true);

    if (Parts.Num() == 0) return false;

    lua_getglobal(L, TCHAR_TO_UTF8(*Parts[0]));
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        return false;
    }

    for (int32 i = 1; i < Parts.Num(); i++)
    {
        FTCHARToUTF8 Convert(*Parts[i]);
        lua_pushlstring(L, Convert.Get(), Convert.Length());
        lua_gettable(L, -2);
        if (!lua_istable(L, -1))
        {
            lua_pop(L, 2);
            return false;
        }
        lua_remove(L, -2);
    }

    return true;
}
