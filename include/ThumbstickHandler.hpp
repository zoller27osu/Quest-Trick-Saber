#pragma once

#include "InputHandler.hpp"
#include "PluginConfig.hpp"
#include "AllEnums.hpp"

#include <string>

class ThumbstickHandler : public InputHandler {
  private:
    Il2CppString* _inputString;

  public:
    ThumbstickHandler(XRNode node, float threshold, ThumbstickDir thumstickDir) : InputHandler(threshold) {
        std::string axis = thumstickDir == ThumbstickDir::Horizontal ? "Horizontal" : "Vertical";
        axis += node == XRNode::LeftHand ? "LeftHand" : "RightHand";
        _inputString = il2cpp_utils::createcsstr(axis);
        IsReversed = PluginConfig::Instance().ReverseThumbstick;
    }

    float GetInputValue() {
        static auto* klass = CRASH_UNLESS(il2cpp_utils::GetClassFromName("UnityEngine", "Input"));
        auto val = CRASH_UNLESS(il2cpp_utils::RunMethod<float>(klass, "GetAxis", _inputString));
        // if (val != 0) logger().debug("ThumbstickHandler input value: %f", val);
        return val;
    }
};