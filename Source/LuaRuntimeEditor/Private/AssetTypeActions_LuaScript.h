#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_LuaScript : public FAssetTypeActions_Base
{
public:
    virtual FText GetName() const override { return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_LuaScript", "Lua Script"); }
    virtual FColor GetTypeColor() const override { return FColor(200, 230, 120); }
    virtual UClass* GetSupportedClass() const override;
    virtual uint32 GetCategories() override;
};

