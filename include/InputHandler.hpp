#pragma once

#include <math.h>
#include "extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"

class InputHandler {
  private:
    float GetActivationValue(float val) {
        float value = abs(val);
        if (IsReversed) return 1.0f - value;
        return value;
    }

  protected:
    bool IsDeactivated = true;
    float Threshold;
    bool IsReversed;

    InputHandler(float threshold) {
        Threshold = threshold;
    }

  public:
    virtual ~InputHandler() {}
    virtual float GetInputValue() = 0;

    bool Activated(float& val) {
        auto value = GetInputValue();
        auto activationValue = GetActivationValue(value);
        val = 0;

        if (activationValue > Threshold) {
            val = IsReversed ? activationValue : value;
            IsDeactivated = false;
            return true;
        }

        return false;
    }

    bool Deactivated() {
        if (GetActivationValue(GetInputValue()) < Threshold && !IsDeactivated) {
            IsDeactivated = true;
            return true;
        }

        return false;
    }
};
