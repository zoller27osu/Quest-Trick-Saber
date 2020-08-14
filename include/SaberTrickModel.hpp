#pragma once

#include "PluginConfig.hpp"
#include "extern/beatsaber-hook/shared/utils/il2cpp-functions.hpp"
#include "extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "extern/beatsaber-hook/shared/utils/utils.h"

class SaberTrickModel {
  public:
    Il2CppObject* Rigidbody = nullptr;
    Il2CppObject* SaberGO;  // GameObject

    SaberTrickModel(Il2CppObject* SaberModel, bool basicSaber) {
        CRASH_UNLESS(SaberModel);
        logger().debug("SaberTrickModel construction!");
        tRigidbody = CRASH_UNLESS(il2cpp_utils::GetSystemType("UnityEngine", "Rigidbody"));
        // il2cpp_utils::LogClass(il2cpp_functions::class_from_system_type(tRigidbody), false);

        SaberGO = OriginalSaberModel = SaberModel;

        if (PluginConfig::Instance().EnableTrickCutting) {
            Rigidbody = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberModel, "GetComponent", tRigidbody));
            if (!Rigidbody) {
                logger().warning("Adding rigidbody to original SaberModel?!");
                Rigidbody = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberModel, "AddComponent", tRigidbody));
            }
            SetupRigidbody(Rigidbody, OriginalSaberModel);
        } else {
            TrickModel = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "Object", "Instantiate", SaberModel));
            CRASH_UNLESS(TrickModel);
            FixBasicTrickSaber(TrickModel, basicSaber);
            AddTrickRigidbody();

            auto* str = CRASH_UNLESS(il2cpp_utils::createcsstr("VRGameCore"));
            auto* vrGameCore = CRASH_UNLESS(il2cpp_utils::RunMethod("UnityEngine", "GameObject", "Find", str));
            auto* vrGameCoreT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(vrGameCore, "transform"));
            auto* trickModelT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(TrickModel, "transform"));
            CRASH_UNLESS(il2cpp_utils::RunMethod(trickModelT, "SetParent", vrGameCoreT));
            CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", false));
        }
        logger().debug("Leaving SaberTrickModel construction!");
    }

    void SetupRigidbody(Il2CppObject* rigidbody, Il2CppObject* model) {
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(rigidbody, "useGravity", false));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(rigidbody, "isKinematic", true));

        static auto set_detectCollisions = (function_ptr_t<void, Il2CppObject*, bool>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_detectCollisions"));
        logger().debug("set_detectCollisions ptr offset: %lX", asOffset(set_detectCollisions));
        set_detectCollisions(rigidbody, false);

        static auto set_maxAngVel = (function_ptr_t<void, Il2CppObject*, float>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_maxAngularVelocity"));
        logger().debug("set_maxAngVel ptr offset: %lX", asOffset(set_maxAngVel));
        set_maxAngVel(rigidbody, 800.0f);

        static auto set_interp = (function_ptr_t<void, Il2CppObject*, int>)CRASH_UNLESS(
            il2cpp_functions::resolve_icall("UnityEngine.Rigidbody::set_interpolation"));
        logger().debug("set_interpolation ptr offset: %lX", asOffset(set_interp));
        set_interp(rigidbody, 1);  // Interpolate
    }

    void AddTrickRigidbody() {
        Rigidbody = CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "AddComponent", tRigidbody));
        CRASH_UNLESS(Rigidbody);
        SetupRigidbody(Rigidbody, TrickModel);
    }

    void FixBasicTrickSaber(Il2CppObject* newSaber, bool basic) {
        if (!basic) return;
        logger().debug("Fixing basic trick saber color!");

        static auto* tSaberModelContainer = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "SaberModelContainer"));
        auto* saberModelContainer = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberGO, "GetComponentInParent", tSaberModelContainer));
        auto* _saberTypeObject = CRASH_UNLESS(il2cpp_utils::GetFieldValue(saberModelContainer, "_saberTypeObject"));
        auto* saberType = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(_saberTypeObject, "saberType"));
        CRASH_UNLESS(saberType);
        auto* saberModelContainerT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(saberModelContainer, "transform"));
        CRASH_UNLESS(saberModelContainerT);

        static auto* tSaberModelController = CRASH_UNLESS(il2cpp_utils::GetSystemType("", "ISaberModelController"));
        auto* saberModelController = CRASH_UNLESS(il2cpp_utils::RunMethod(newSaber, "GetComponent", tSaberModelController));
        CRASH_UNLESS(saberModelController);

        auto* origModelController = CRASH_UNLESS(il2cpp_utils::RunMethod(SaberGO, "GetComponent", tSaberModelController));
        auto* colorMgr = CRASH_UNLESS(il2cpp_utils::GetFieldValue(origModelController, "_colorManager"));
        CRASH_UNLESS(colorMgr);
        CRASH_UNLESS(il2cpp_utils::SetFieldValue(saberModelController, "_colorManager", colorMgr));

        auto* glows = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Il2CppArray*>(saberModelController, "_setSaberGlowColors"));
        logger().info("_setSaberGlowColors.length: %u", il2cpp_functions::array_length(glows));
        for (int i = 0; i < il2cpp_functions::array_length(glows); i++) {
            auto* obj = il2cpp_array_get(glows, Il2CppObject*, i);
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(obj, "_colorManager", colorMgr));
        }

        auto* fakeGlows = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Il2CppArray*>(saberModelController, "_setSaberFakeGlowColors"));
        logger().info("_setSaberFakeGlowColors.length: %u", il2cpp_functions::array_length(fakeGlows));
        for (int i = 0; i < il2cpp_functions::array_length(fakeGlows); i++) {
            auto* obj = il2cpp_array_get(fakeGlows, Il2CppObject*, i);
            CRASH_UNLESS(il2cpp_utils::SetFieldValue(obj, "_colorManager", colorMgr));
        }

        CRASH_UNLESS(il2cpp_utils::RunMethod(saberModelController, "Init", saberModelContainerT, saberType));
    }

    void ChangeToTrickModel() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", true));
        auto* trickT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(TrickModel, "transform"));
        auto* origT = CRASH_UNLESS(il2cpp_utils::GetPropertyValue(OriginalSaberModel, "transform"));
        auto pos = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Vector3>(origT, "position"));
        auto rot = CRASH_UNLESS(il2cpp_utils::GetPropertyValue<Quaternion>(origT, "rotation"));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(trickT, "position", pos));
        CRASH_UNLESS(il2cpp_utils::SetPropertyValue(trickT, "rotation", rot));
        CRASH_UNLESS(il2cpp_utils::RunMethod(OriginalSaberModel, "SetActive", false));
        SaberGO = TrickModel;
    }

    void ChangeToActualSaber() {
        if (PluginConfig::Instance().EnableTrickCutting) return;
        CRASH_UNLESS(il2cpp_utils::RunMethod(OriginalSaberModel, "SetActive", true));
        CRASH_UNLESS(il2cpp_utils::RunMethod(TrickModel, "SetActive", false));
        SaberGO = OriginalSaberModel;
    }


  private:
    Il2CppReflectionType* tRigidbody = nullptr;
    Il2CppObject* OriginalSaberModel = nullptr;  // GameObject
    Il2CppObject* TrickModel = nullptr;          // GameObject
};
