#include "../include/main.hpp"
#include "../include/PluginConfig.hpp"
#include "extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "extern/bs-utils/shared/utils.hpp"

const PluginConfig::ConfigDefaultsMap PluginConfig::configDefaults = InitializeConfigDefaults();
const PluginConfig::EnumValuesMap PluginConfig::enumValues = GetEnumValuesMap();

void PluginConfig::Reload() {
    config().Reload();
    CheckRegenConfig(config());
    auto& doc = config().config;

    // TODO: clamp negative numbers to 0?
    TriggerAction = (TrickAction)ValidatedEnumMember(doc, "TriggerAction");
    GripAction = (TrickAction)ValidatedEnumMember(doc, "GripAction");
    ThumbstickAction = (TrickAction)ValidatedEnumMember(doc, "ThumbstickAction");
    ThumbstickDirection = (ThumbstickDir)ValidatedEnumMember(doc, "ThumbstickDirection");

    ReverseTrigger = GetOrAddDefault(doc, "ReverseTrigger").GetBool();
    ReverseGrip = GetOrAddDefault(doc, "ReverseGrip").GetBool();
    ReverseThumbstick = GetOrAddDefault(doc, "ReverseThumbstick").GetBool();

    TriggerThreshold = GetOrAddDefault(doc, "TriggerThreshold").GetFloat();
    GripThreshold = GetOrAddDefault(doc, "GripThreshold").GetFloat();
    ThumbstickThreshold = GetOrAddDefault(doc, "ThumbstickThreshold").GetFloat();

    IsVelocityDependent = GetOrAddDefault(doc, "IsSpinVelocityDependent").GetBool();
    SpinSpeed = GetOrAddDefault(doc, "SpinSpeed").GetFloat();
    SpinDirection = (SpinDir)ValidatedEnumMember(doc, "SpinDirection");
    ControllerSnapThreshold = GetOrAddDefault(doc, "ControllerSnapThreshold").GetFloat();
    if (ControllerSnapThreshold < 0) {
        logger().error("config's ControllerSnapThreshold was negative (specifically, %f)! Setting to default.", ControllerSnapThreshold);
        ControllerSnapThreshold = configDefaults.at("ControllerSnapThreshold").GetFloat();
        // TODO: write new value to config?
    }

    ThrowVelocity = GetOrAddDefault(doc, "ThrowVelocity").GetFloat();
    EnableTrickCutting = GetOrAddDefault(doc, "EnableTrickCutting").GetBool();
    CompleteRotationMode = GetOrAddDefault(doc, "CompleteRotationMode").GetBool();
    ReturnSpeed = GetOrAddDefault(doc, "ReturnSpeed").GetFloat();

    SlowmoDuringThrow = GetOrAddDefault(doc, "SlowmoDuringThrow").GetBool();
    SlowmoAmount = GetOrAddDefault(doc, "SlowmoAmount").GetFloat();

    // "Advanced" options
    VelocityBufferSize = GetOrAddDefault(doc, "VelocityBufferSize").GetInt();
    SlowmoStepAmount = GetOrAddDefault(doc, "SlowmoStepAmount").GetFloat();

    config().Write();

    if (EnableTrickCutting || SlowmoDuringThrow) {
        bs_utils::Submission::disable(modInfo);
    } else {
        bs_utils::Submission::enable(modInfo);
    }
}
