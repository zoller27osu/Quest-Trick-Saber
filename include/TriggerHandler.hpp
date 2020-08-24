#pragma once

#include "InputHandler.hpp"
#include "PluginConfig.hpp"
#include "AllEnums.hpp"

class TriggerHandler : public InputHandler {
  private:
    Il2CppString* _inputString;

  public:
    TriggerHandler(XRNode node, float threshold) : InputHandler(threshold) {
        // axis names are from HMLib's VRControllersInputManager
        auto str = (node == XRNode::LeftHand) ? "TriggerLeftHand" : "TriggerRightHand";
        _inputString = il2cpp_utils::createcsstr(str, il2cpp_utils::StringType::Manual);
        IsReversed = PluginConfig::Instance().ReverseTrigger;
    }

    float GetInputValue() {
        static auto* klass = CRASH_UNLESS(il2cpp_utils::GetClassFromName("UnityEngine", "Input"));
        auto val = CRASH_UNLESS(il2cpp_utils::RunMethod<float>(klass, "GetAxis", _inputString));
        // if (val != 0) logger().debug("TriggerHandler input value: %f", val);
        return val;
    }

    ~TriggerHandler() {
        free(_inputString);
    }
};