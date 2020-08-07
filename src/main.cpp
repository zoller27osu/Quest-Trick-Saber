#include "../include/main.hpp"
#include "../include/PluginConfig.hpp"
#include "../include/TrickManager.hpp"

const Logger& logger()
{
    static const Logger& logger(modInfo);
    return logger;
}

extern "C" void setup(ModInfo& info)
{
    info.id      = "TrickSaber";
    info.version = "0.2.0";
    modInfo      = info;
    logger().info("Leaving setup!");
}

TrickManager leftSaber;
TrickManager rightSaber;

MAKE_HOOK_OFFSETLESS(Saber_Start, void, Il2CppObject* self) {
    TrickManager::Clear();
    PluginConfig::Instance().Reload();  // TODO: this only needs to be called once on song start, not once for each saber
    Saber_Start(self);
    int saberType = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(self, "saberType"));
    logger().debug("SaberType: %i", saberType);
    if(saberType == 0){
        logger().debug("Left?");
        leftSaber.VRController = CRASH_UNLESS(il2cpp_utils::GetFieldValue(self, "_vrController"));
        leftSaber.Saber = self;
        leftSaber._isLeftSaber = true;
        leftSaber.other = &rightSaber;
        leftSaber.Start();
    } else {
        logger().debug("Right?");
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
    logger().debug("OVRInput_Update!");
    OVRInput_Update(self);
}

MAKE_HOOK_OFFSETLESS(FixedUpdate, void, Il2CppObject* self) {
    FixedUpdate(self);
    TrickManager::FixedUpdate();
}

MAKE_HOOK_OFFSETLESS(Pause, void, Il2CppObject* self) {
    leftSaber.PauseTricks();
    rightSaber.PauseTricks();
    Pause(self);
}

MAKE_HOOK_OFFSETLESS(Resume, void, Il2CppObject* self) {
    Resume(self);
    leftSaber.ResumeTricks();
    rightSaber.ResumeTricks();
}

extern "C" void load() {
    PluginConfig::Init();
    // TODO: config menus
    logger().info("Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(Saber_ManualUpdate, il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));
    INSTALL_HOOK_OFFSETLESS(Saber_Start, il2cpp_utils::FindMethod("", "Saber", "Start"));

    INSTALL_HOOK_OFFSETLESS(FixedUpdate, il2cpp_utils::FindMethod("", "OculusVRHelper", "FixedUpdate"));
    // INSTALL_HOOK_OFFSETLESS(LateUpdate, il2cpp_utils::FindMethod("", "VRPlatformHelper", "LateUpdate"));

    INSTALL_HOOK_OFFSETLESS(Pause, il2cpp_utils::FindMethod("", "GamePause", "Pause"));
    INSTALL_HOOK_OFFSETLESS(Resume, il2cpp_utils::FindMethod("", "GamePause", "Resume"));
    logger().info("Installed all hooks!");
}
