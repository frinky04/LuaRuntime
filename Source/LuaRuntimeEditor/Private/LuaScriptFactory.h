#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetTypeCategories.h"
#include "LuaScriptFactory.generated.h"

UCLASS()
class ULuaScriptFactory : public UFactory
{
    GENERATED_BODY()
public:
    ULuaScriptFactory();

    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual FText GetDisplayName() const override { return NSLOCTEXT("Lua", "LuaScriptFactoryDisplayName", "Lua Script"); }
    virtual uint32 GetMenuCategories() const override { return EAssetTypeCategories::Misc; }
    virtual bool ShouldShowInNewMenu() const override { return true; }
};
