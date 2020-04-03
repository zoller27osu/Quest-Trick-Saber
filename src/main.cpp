#include "../include/main.hpp"
#include "../include/TrickManager.hpp"
TrickManager leftSaber;
TrickManager rightSaber;
MAKE_HOOK_OFFSETLESS(Saber_Start, void, Il2CppObject* self){
    
    int saberType;
    il2cpp_utils::RunMethod(&saberType, self, "get_saberType");
    log(DEBUG, "SaberType: %i", saberType);
    if(saberType == 0){
        log(DEBUG, "Left?");
        leftSaber.Controller = il2cpp_utils::GetFieldValue(self, "_vrController");
        leftSaber.Saber = self;
        leftSaber._isLeftSaber = true;
        leftSaber.Start();
    } else {
        rightSaber.Controller = il2cpp_utils::GetFieldValue(self, "_vrController");
        rightSaber.Saber = self;
        rightSaber.Start();
    }
    Saber_Start(self);
    
}


MAKE_HOOK_OFFSETLESS(Saber_ManualUpdate, void, Il2CppObject* self){
    Saber_ManualUpdate(self);
    leftSaber.Update();
    //rightSaber.LogEverything();
    rightSaber.Update();
}


extern "C" void load() {
    log(INFO, "Hello from il2cpp_init!");
    log(INFO, "Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(Saber_ManualUpdate, il2cpp_utils::FindMethodUnsafe("", "Saber", "ManualUpdate", 0));
    INSTALL_HOOK_OFFSETLESS(Saber_Start, il2cpp_utils::FindMethodUnsafe("", "Saber", "Start", 0));
    log(INFO, "Installed all hooks!");
}