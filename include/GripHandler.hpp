#pragma once

#include <functional>

#include "InputHandler.hpp"
#include "PluginConfig.hpp"
#include "AllEnums.hpp"

class GripHandler : public InputHandler {
  private:
    const Controller _oculusController;
    Il2CppObject* _controllerInputDevice;  // UnityEngine.XR.InputDevice

    typedef float (GripHandler::*MemFn)();
    MemFn _valueFunc;

    float GetValueSteam() {
        static auto* commonUsages = CRASH_UNLESS(il2cpp_utils::GetClassFromName("UnityEngine.XR", "CommonUsages"));
        auto* grip = CRASH_UNLESS(il2cpp_utils::GetFieldValue(commonUsages, "grip"));
        float outvar;
        // argument types arg important for finding the correct match
        if (CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(_controllerInputDevice, "TryGetFeatureValue", grip, outvar))) {
            return outvar;
        }
        return 0;
    }

    //CommomUsages doesn't work well with Touch Controllers, so we need to use the Oculus function for them
    float GetValueOculus() {
        static auto primaryHandTrigger = CRASH_UNLESS(il2cpp_utils::GetFieldValue<Axis1D>(
            "", "OVRInput/Axis1D", "PrimaryHandTrigger"));
        static auto* ovrInput = CRASH_UNLESS(il2cpp_utils::GetClassFromName("", "OVRInput"));
        // let il2cpp_utils cache the method
        return CRASH_UNLESS(il2cpp_utils::RunMethod<float>(ovrInput, "Get", primaryHandTrigger, _oculusController));
    }

  public:
    GripHandler(VRSystem vrSystem, Controller oculusController, Il2CppObject* controllerInputDevice, float threshold)
    : InputHandler(threshold), _oculusController(oculusController), _controllerInputDevice(controllerInputDevice) {
        _valueFunc = (vrSystem == VRSystem::Oculus) ? &GripHandler::GetValueOculus : &GripHandler::GetValueSteam;

        IsReversed = PluginConfig::Instance().ReverseGrip;
    }

    #define CALL_MEMFN_ON_PTR(ptr,memFn)  ((ptr)->*(memFn))

    float GetInputValue() {
        auto val = CALL_MEMFN_ON_PTR(this, _valueFunc)();
        // if (val != 0) logger().debug("GripHandler input value: %f", val);
        return val;
    }
};