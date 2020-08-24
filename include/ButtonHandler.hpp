#pragma once

#include "InputHandler.hpp"
#include "PluginConfig.hpp"
#include "AllEnums.hpp"

class ButtonHandler : public InputHandler {
  private:
    const Controller _oculusController;
    const Button _button;

  public:
    ButtonHandler(Controller oculusController, Button button) 
      : InputHandler(0.5f), _button(button), _oculusController(oculusController)
    {
        IsReversed = button == Button::One ? PluginConfig::Instance().ReverseButtonOne
          : PluginConfig::Instance().ReverseButtonTwo;
    }

    float GetInputValue() {
        static auto* ovrInput = CRASH_UNLESS(il2cpp_utils::GetClassFromName("", "OVRInput"));
        // let il2cpp_utils cache the method
        return CRASH_UNLESS(il2cpp_utils::RunMethod<bool>(ovrInput, "Get", _button, _oculusController)) ? 1.0f : 0.0f;
    }
};
