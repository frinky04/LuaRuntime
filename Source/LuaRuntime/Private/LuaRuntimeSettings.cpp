#include "LuaRuntimeSettings.h"

ULuaRuntimeSettings::ULuaRuntimeSettings()
{
    // Printing defaults
    bOnScreenPrintEnabled = true;
    OnScreenPrintDuration = 4.0f;
    OnScreenPrintColor = FLinearColor(0.2f, 1.0f, 0.3f, 1.0f);
    bAllowOnScreenPrintInShipping = false;

    // Execution defaults
    DefaultMemoryLimitKB = 1024;
    DefaultTimeoutMs = 50;
    DefaultHookInterval = 1000;
}

