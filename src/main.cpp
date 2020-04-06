#include "../include/main.hpp"
#include "../include/TrickManager.hpp"

TrickManager leftSaber;
TrickManager rightSaber;

MAKE_HOOK_OFFSETLESS(Saber_Start, void, Il2CppObject* self) {
    Saber_Start(self);
    int saberType = *CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(self, "saberType"));
    log(DEBUG, "SaberType: %i", saberType);
    if(saberType == 0){
        log(DEBUG, "Left?");
        leftSaber.VRController = *CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_vrController"));
        leftSaber.Saber = self;
        leftSaber._isLeftSaber = true;
        leftSaber.Start();
    } else {
        log(DEBUG, "Right?");
        rightSaber.VRController = *CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_vrController"));
        rightSaber.Saber = self;
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

extern "C" void load() {
    log(INFO, "Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(Saber_ManualUpdate, il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));
    INSTALL_HOOK_OFFSETLESS(Saber_Start, il2cpp_utils::FindMethod("", "Saber", "Start"));
    log(INFO, "Installed all hooks!");
}
