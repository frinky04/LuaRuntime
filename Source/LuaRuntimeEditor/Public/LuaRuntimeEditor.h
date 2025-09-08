#pragma once

#include "Modules/ModuleManager.h"

class FLuaRuntimeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    TArray<TSharedPtr<class IAssetTypeActions>> RegisteredAssetTypeActions;
};
