#include "../include/main.hpp"
#include "../include/PluginConfig.hpp"
#include "../include/TrickManager.hpp"

const Logger& logger() {
    static const Logger& logger(modInfo);
    return logger;
}

extern "C" void setup(ModInfo& info) {
    info.id      = "TrickSaber";
    info.version = "0.2.0";
    modInfo      = info;
    logger().info("Leaving setup!");
}

Il2CppObject* FakeSaber = nullptr;
Il2CppObject* RealSaber = nullptr;
TrickManager leftSaber;
TrickManager rightSaber;

MAKE_HOOK_OFFSETLESS(GameScenesManager_PushScenes, void, Il2CppObject* self, Il2CppObject* scenesTransitionSetupData,
        float minDuration, Il2CppObject* afterMinDurationCallback, Il2CppObject* finishCallback) {
    logger().debug("GameScenesManager_PushScenes");
    GameScenesManager_PushScenes(self, scenesTransitionSetupData, minDuration, afterMinDurationCallback, finishCallback);
    FakeSaber = nullptr;
    RealSaber = nullptr;
    PluginConfig::Instance().Reload();
    TrickManager::StaticClear();
    leftSaber.Clear();
    rightSaber.Clear();
    logger().debug("Leaving GameScenesManager_PushScenes");
}

MAKE_HOOK_OFFSETLESS(Saber_Start, void, Il2CppObject* self) {
    logger().debug("Saber_Start");
    Saber_Start(self);
    logger().debug("Saber_Start original called");
    int saberType = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(self, "saberType"));
    logger().debug("SaberType: %i", saberType);
    if (saberType == 0) {
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
    RealSaber = self;
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

static std::vector<Il2CppReflectionType*> tBurnTypes;
void DisableBurnMarks(int saberType) {
    if (!FakeSaber) {
        static auto* tSaber = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "Saber"));
        auto* core = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "GameObject", "Find", il2cpp_utils::createcsstr("GameCore")));
        FakeSaber = CRASH_UNLESS(il2cpp_utils::RunMethod(core, "AddComponent", tSaber));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(FakeSaber, "enabled", false));
        logger().info("FakeSaber.isActiveAndEnabled: %i",
            CRASH_UNLESS(il2cpp_utils::GetPropertyValue<bool>(FakeSaber, "isActiveAndEnabled")));

        auto* saberTypeObj = CRASH_UNLESS(il2cpp_utils::GetFieldValue(RealSaber, "_saberType"));
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(FakeSaber, "_saberType", saberTypeObj));
        logger().info("FakeSaber SaberType: %i", CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(FakeSaber, "saberType")));
    }
    for (auto* type : tBurnTypes) {
        auto* components = CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(
            "UnityEngine", "Object", "FindObjectsOfType", type));
        for (int i = 0; i < components->Length(); i++) {
            auto* sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Il2CppObject*>*>(components->values[i], "_sabers"));
            sabers->values[saberType] = FakeSaber;
        }
    }
}

void EnableBurnMarks(int saberType) {
    for (auto* type : tBurnTypes) {
        auto* components = CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(
            "UnityEngine", "Object", "FindObjectsOfType", type));
        for (int i = 0; i < components->Length(); i++) {
            auto* playerController = CRASH_UNLESS(il2cpp_utils::GetFieldValue(components->values[i], "_playerController"));
            auto* sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Il2CppObject*>*>(components->values[i], "_sabers"));
            sabers->values[saberType] = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(playerController,
                saberType ? "rightSaber" : "leftSaber"));;
        }
    }
}

MAKE_HOOK_OFFSETLESS(OVRInput_Update, void, Il2CppObject* self) {
    logger().debug("OVRInput_Update!");
    OVRInput_Update(self);
}

MAKE_HOOK_OFFSETLESS(FixedUpdate, void, Il2CppObject* self) {
    FixedUpdate(self);
    TrickManager::StaticFixedUpdate();
    leftSaber.FixedUpdate();
    rightSaber.FixedUpdate();
}

MAKE_HOOK_OFFSETLESS(Pause, void, Il2CppObject* self) {
    Pause(self);
    leftSaber.PauseTricks();
    rightSaber.PauseTricks();
}

MAKE_HOOK_OFFSETLESS(Resume, void, Il2CppObject* self) {
    leftSaber.ResumeTricks();
    rightSaber.ResumeTricks();
    Resume(self);
}

extern "C" void load() {
    PluginConfig::Init();
    // TODO: config menus
    logger().info("Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(GameScenesManager_PushScenes, il2cpp_utils::FindMethodUnsafe("", "GameScenesManager", "PushScenes", 4));
    INSTALL_HOOK_OFFSETLESS(Saber_Start, il2cpp_utils::FindMethod("", "Saber", "Start"));
    INSTALL_HOOK_OFFSETLESS(Saber_ManualUpdate, il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));

    INSTALL_HOOK_OFFSETLESS(FixedUpdate, il2cpp_utils::FindMethod("", "OculusVRHelper", "FixedUpdate"));
    // INSTALL_HOOK_OFFSETLESS(LateUpdate, il2cpp_utils::FindMethod("", "SaberBurnMarkSparkles", "LateUpdate"));

    INSTALL_HOOK_OFFSETLESS(Pause, il2cpp_utils::FindMethod("", "GamePause", "Pause"));
    INSTALL_HOOK_OFFSETLESS(Resume, il2cpp_utils::FindMethod("", "GamePause", "Resume"));

    tBurnTypes.push_back(CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberBurnMarkArea")));
    tBurnTypes.push_back(CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberBurnMarkSparkles")));
    tBurnTypes.push_back(CRASH_UNLESS(il2cpp_utils::GetSystemType("", "ObstacleSaberSparkleEffectManager")));

    logger().info("Installed all hooks!");
}
