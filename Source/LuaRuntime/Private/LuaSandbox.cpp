#include "LuaSandbox.h"
#include "LuaRuntime.h"
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

    // pcall with 0 args, and let chunk return discarded
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
