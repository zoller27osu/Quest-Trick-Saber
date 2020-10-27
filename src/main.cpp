#include "../include/main.hpp"
#include "../include/PluginConfig.hpp"
#include "../include/TrickManager.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-functions.hpp"

const Logger& logger() {
    static const Logger& logger(modInfo);
    return logger;
}

extern "C" void setup(ModInfo& info) {
    info.id      = "TrickSaber";
    info.version = "0.2.3";
    modInfo      = info;
    logger().info("Leaving setup!");
}

void ListGameObjects(Il2CppObject* root, std::string_view prefix) {
    auto* go = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(root, "gameObject"));
    auto* rootT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(root, "transform"));

    auto tag = to_utf8(csstrtostr(CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Il2CppString*>(rootT, "tag"))));
    auto name = to_utf8(csstrtostr(CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Il2CppString*>(root, "name"))));
    bool activeSelf = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<bool>(go, "activeSelf"));
    bool activeInHierarchy = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<bool>(go, "activeInHierarchy"));
    logger().debug("%s%s%sname %s, tag %s", activeSelf ? "!" : " ", activeInHierarchy ? "*" : " ",
        prefix.data(), name.c_str(), tag.c_str());

    auto childCount = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(rootT, "childCount"));
    std::string childPrefix(prefix);
    childPrefix += "  ";
    for (int i = 0; i < childCount; i++) {
        auto* child = CRASH_UNLESS(il2cpp_utils::RunMethod(rootT, "GetChild", i));
        ListGameObjects(child, childPrefix);
    }
}

Il2CppObject* FakeSaber = nullptr;
Il2CppObject* RealSaber = nullptr;
TrickManager leftSaber;
TrickManager rightSaber;

bool AttachForSpin;
bool SpinIsRelativeToVRController;
void UpdateForConfig() {
    AttachForSpin = TrailFollowsSaberComponent && !PluginConfig::Instance().EnableTrickCutting;
    SpinIsRelativeToVRController = AttachForSpin;
}

MAKE_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, void, Scene scene, int mode) {
    if (auto nameOpt = il2cpp_utils::GetPropertyValue<Il2CppString*>(scene, "name")) {
        auto* name = *nameOpt;
        auto str = to_utf8(csstrtostr(name));
        logger().debug("Scene name internal: %s", str.c_str());
        // sabersEnabled = (str == "GameCore");
    }
    FakeSaber = nullptr;
    RealSaber = nullptr;
    TrickManager::StaticClear();
    leftSaber.Clear();
    rightSaber.Clear();
    SceneManager_Internal_SceneLoaded(scene, mode);
    fakeSabers.clear();
}

MAKE_HOOK_OFFSETLESS(GameScenesManager_PushScenes, void, Il2CppObject* self, Il2CppObject* scenesTransitionSetupData,
        float minDuration, Il2CppObject* afterMinDurationCallback, Il2CppObject* finishCallback) {
    logger().debug("GameScenesManager_PushScenes");
    GameScenesManager_PushScenes(self, scenesTransitionSetupData, minDuration, afterMinDurationCallback, finishCallback);
    PluginConfig::Instance().Reload();
    UpdateForConfig();
    logger().debug("Leaving GameScenesManager_PushScenes");
}

MAKE_HOOK_OFFSETLESS(Saber_Start, void, Il2CppObject* self) {
    logger().debug("Saber_Start: %p", self);
    Saber_Start(self);
    logger().debug("Saber_Start original called");
    if (fakeSabers.contains(self)) return;
    
    int saberType = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<int>(self, "saberType"));
    logger().debug("SaberType: %i", saberType);

    static auto* tVRController = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "VRController"));
    auto* myController = RET_V_UNLESS(il2cpp_utils::RunMethod(self, "GetComponent", tVRController));

    if (saberType == 0) {
        logger().debug("Left?");
        leftSaber.VRController = myController;
        leftSaber.Saber = self;
        leftSaber._isLeftSaber = true;
        leftSaber.other = &rightSaber;
        leftSaber.Start();
    } else {
        logger().debug("Right?");
        rightSaber.VRController = myController;
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

void DisableBurnMarks(int saberType) {
    if (TrailFollowsSaberComponent) return;
    if (!FakeSaber) {
        static auto* tSaber = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "Saber"));
        auto* core = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "GameObject", "Find", il2cpp_utils::createcsstr("GameCore")));
        FakeSaber = CRASH_UNLESS(il2cpp_utils::RunMethod(core, "AddComponent", tSaber));
        fakeSabers.insert(FakeSaber);
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
        for (il2cpp_array_size_t i = 0; i < components->Length(); i++) {
            auto* sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Il2CppObject*>*>(components->values[i], "_sabers"));
            sabers->values[saberType] = FakeSaber;
        }
    }
}

void EnableBurnMarks(int saberType) {
    if (TrailFollowsSaberComponent) return;
    for (auto* type : tBurnTypes) {
        auto* components = CRASH_UNLESS(il2cpp_utils::RunMethod<Array<Il2CppObject*>*>(
            "UnityEngine", "Object", "FindObjectsOfType", type));
        for (il2cpp_array_size_t i = 0; i < components->Length(); i++) {
            auto* saberManager = CRASH_UNLESS(il2cpp_utils::GetFieldValue(components->values[i], "_saberManager"));
            auto* sabers = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Array<Il2CppObject*>*>(components->values[i], "_sabers"));
            sabers->values[saberType] = CRASH_UNLESS(il2cpp_utils::GetFieldValue(saberManager,
                saberType ? "_rightSaber" : "_leftSaber"));
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
}

MAKE_HOOK_OFFSETLESS(Pause, void, Il2CppObject* self) {
    leftSaber.PauseTricks();
    rightSaber.PauseTricks();
    Pause(self);
    TrickManager::StaticPause();
    logger().debug("pause: %i", CRASH_UNLESS(il2cpp_utils::GetFieldValue<bool>(self, "_pause")));
}

MAKE_HOOK_OFFSETLESS(Resume, void, Il2CppObject* self) {
    Resume(self);
    TrickManager::StaticResume();
    leftSaber.ResumeTricks();
    rightSaber.ResumeTricks();
    logger().debug("pause: %i", CRASH_UNLESS(il2cpp_utils::GetFieldValue<bool>(self, "_pause")));
}

MAKE_HOOK_OFFSETLESS(SaberClashChecker_AreSabersClashing, bool, Il2CppObject* self, Vector3& clashingPoint) {
    if (TrickManager::disableClashing) return false;
    return SaberClashChecker_AreSabersClashing(self, clashingPoint);
}

extern "C" void load() {
    PluginConfig::Init();
    UpdateForConfig();
    // TODO: config menus
    logger().info("Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(SceneManager_Internal_SceneLoaded, il2cpp_utils::FindMethodUnsafe("UnityEngine.SceneManagement", "SceneManager", "Internal_SceneLoaded", 2));
    INSTALL_HOOK_OFFSETLESS(GameScenesManager_PushScenes, il2cpp_utils::FindMethodUnsafe("", "GameScenesManager", "PushScenes", 4));
    INSTALL_HOOK_OFFSETLESS(Saber_Start, il2cpp_utils::FindMethod("", "Saber", "Start"));
    INSTALL_HOOK_OFFSETLESS(Saber_ManualUpdate, il2cpp_utils::FindMethod("", "Saber", "ManualUpdate"));

    INSTALL_HOOK_OFFSETLESS(FixedUpdate, il2cpp_utils::FindMethod("", "OculusVRHelper", "FixedUpdate"));
    // INSTALL_HOOK_OFFSETLESS(LateUpdate, il2cpp_utils::FindMethod("", "SaberBurnMarkSparkles", "LateUpdate"));

    INSTALL_HOOK_OFFSETLESS(Pause, il2cpp_utils::FindMethod("", "GamePause", "Pause"));
    INSTALL_HOOK_OFFSETLESS(Resume, il2cpp_utils::FindMethod("", "GamePause", "Resume"));

    tBurnTypes.insert(CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberBurnMarkArea")));
    tBurnTypes.insert(CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberBurnMarkSparkles")));
    tBurnTypes.insert(CRASH_UNLESS(il2cpp_utils::GetSystemType("", "ObstacleSaberSparkleEffectManager")));

    INSTALL_HOOK_OFFSETLESS(SaberClashChecker_AreSabersClashing,
        il2cpp_utils::FindMethodUnsafe("", "SaberClashChecker", "AreSabersClashing", 1));

    logger().info("Installed all hooks!");
}
