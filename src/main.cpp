#include "../include/main.hpp"
#include "../include/PluginConfig.hpp"
#include "../include/TrickManager.hpp"

TrickManager leftSaber;
TrickManager rightSaber;

MAKE_HOOK_OFFSETLESS(Saber_Start, void, Il2CppObject* self) {
    TrickManager::Clear();
    PluginConfig::Instance().Reload();  // TODO: this only needs to be called once on song start, not once for each saber
    Saber_Start(self);
    int saberType = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(self, "saberType"));
    log(DEBUG, "SaberType: %i", saberType);
    if(saberType == 0){
        log(DEBUG, "Left?");
        leftSaber.VRController = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_vrController"));
        leftSaber.Saber = self;
        leftSaber._isLeftSaber = true;
        leftSaber.other = &rightSaber;
        leftSaber.Start();
    } else {
        log(DEBUG, "Right?");
        rightSaber.VRController = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_vrController"));
        rightSaber.Saber = self;
        rightSaber.other = &leftSaber;
        rightSaber.Start();
    }
}

MAKE_HOOK_OFFSETLESS(Saber_ManualUpdate, void, Il2CppObject* self) {
    Saber_ManualUpdate(self);
    if (self == leftSaber.Saber) {
        leftSaber.Update();
    } else if (self == rightSaber.Saber) {
        // rightSaber.LogEverything();
        rightSaber.Update();
    }
}

MAKE_HOOK_OFFSETLESS(OVRInput_Update, void, Il2CppObject* self) {
    log(DEBUG, "OVRInput_Update!");
    OVRInput_Update(self);
}

MAKE_HOOK_OFFSETLESS(FixedUpdate, void, Il2CppObject* self) {
    FixedUpdate(self);
    TrickManager::FixedUpdate();
}

extern "C" void load() {
    PluginConfig::Init();
    // TODO: config menus
    log(INFO, "Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(Saber_ManualUpdate, il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));
    INSTALL_HOOK_OFFSETLESS(Saber_Start, il2cpp_utils::FindMethod("", "Saber", "Start"));

    INSTALL_HOOK_OFFSETLESS(FixedUpdate, il2cpp_utils::FindMethod("", "OculusVRHelper", "FixedUpdate"));
    // INSTALL_HOOK_OFFSETLESS(LateUpdate, il2cpp_utils::FindMethod("", "VRPlatformHelper", "LateUpdate"));
    log(INFO, "Installed all hooks!");
}
