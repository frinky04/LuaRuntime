#include "LuaRuntimeEditor.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypeActions_LuaScript.h"

#define LOCTEXT_NAMESPACE "FLuaRuntimeEditorModule"

void FLuaRuntimeEditorModule::StartupModule()
{
    // Register LuaScript asset actions
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    TSharedRef<IAssetTypeActions> Actions = MakeShareable(new FAssetTypeActions_LuaScript());
    AssetTools.RegisterAssetTypeActions(Actions);
    RegisteredAssetTypeActions.Add(Actions);
}

void FLuaRuntimeEditorModule::ShutdownModule()
{
    if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
    {
        IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
        for (const TSharedPtr<IAssetTypeActions>& Actions : RegisteredAssetTypeActions)
        {
            if (Actions.IsValid())
            {
                AssetTools.UnregisterAssetTypeActions(Actions.ToSharedRef());
            }
        }
        RegisteredAssetTypeActions.Empty();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLuaRuntimeEditorModule, LuaRuntimeEditor)
