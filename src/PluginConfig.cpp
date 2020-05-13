#include "../include/PluginConfig.hpp"
#include "../extern/beatsaber-hook/shared/utils/il2cpp-utils.hpp"

static bool processScoreHooked = false;
static bool disableScores = false;
MAKE_HOOK_OFFSETLESS(SoloFreePlayFlowCoordinator_ProcessScore, void, Il2CppObject* self, Il2CppObject* playerLevelStats, 
		Il2CppObject* levelCompletionResults, Il2CppObject* difficultyBeatmap) {
	if (disableScores) {
        log(DEBUG, "Blocking vanilla score processing!");
		return;
	}
    log(DEBUG, "Allowing vanilla score processing!");
	SoloFreePlayFlowCoordinator_ProcessScore(self, playerLevelStats, levelCompletionResults, difficultyBeatmap);
}

const PluginConfig::ConfigDefaultsMap PluginConfig::configDefaults = InitializeConfigDefaults();
const PluginConfig::EnumValuesMap PluginConfig::enumValues = GetEnumValuesMap();

void PluginConfig::Reload() {
    Configuration::Reload();
    CheckRegenConfig(config);

    // TODO: clamp negative numbers to 0?
    TriggerAction = (TrickAction)ValidatedEnumMember(config, "TriggerAction");
    GripAction = (TrickAction)ValidatedEnumMember(config, "GripAction");
    ThumbstickAction = (TrickAction)ValidatedEnumMember(config, "ThumbstickAction");
    ThumbstickDirection = (ThumbstickDir)ValidatedEnumMember(config, "ThumbstickDirection");

    ReverseTrigger = GetOrAddDefault(config, "ReverseTrigger").GetBool();
    ReverseGrip = GetOrAddDefault(config, "ReverseGrip").GetBool();
    ReverseThumbstick = GetOrAddDefault(config, "ReverseThumbstick").GetBool();

    TriggerThreshold = GetOrAddDefault(config, "TriggerThreshold").GetFloat();
    GripThreshold = GetOrAddDefault(config, "GripThreshold").GetFloat();
    ThumbstickThreshold = GetOrAddDefault(config, "ThumbstickThreshold").GetFloat();

    IsVelocityDependent = GetOrAddDefault(config, "IsSpinVelocityDependent").GetBool();
    SpinSpeed = GetOrAddDefault(config, "SpinSpeed").GetFloat();
    SpinDirection = (SpinDir)ValidatedEnumMember(config, "SpinDirection");
    ControllerSnapThreshold = GetOrAddDefault(config, "ControllerSnapThreshold").GetFloat();
    if (ControllerSnapThreshold < 0) {
        log(ERROR, "config's ControllerSnapThreshold was negative (specifically, %f)! Setting to default.", ControllerSnapThreshold);
        ControllerSnapThreshold = configDefaults.at("ControllerSnapThreshold").GetFloat();
        // TODO: write new value to config?
    }

    ThrowVelocity = GetOrAddDefault(config, "ThrowVelocity").GetFloat();
    EnableTrickCutting = GetOrAddDefault(config, "EnableTrickCutting").GetBool();
    CompleteRotationMode = GetOrAddDefault(config, "CompleteRotationMode").GetBool();
    ReturnSpeed = GetOrAddDefault(config, "ReturnSpeed").GetFloat();

    SlowmoDuringThrow = GetOrAddDefault(config, "SlowmoDuringThrow").GetBool();
    SlowmoAmount = GetOrAddDefault(config, "SlowmoAmount").GetFloat();

    // "Advanced" options
    VelocityBufferSize = GetOrAddDefault(config, "VelocityBufferSize").GetInt();
    SlowmoStepAmount = GetOrAddDefault(config, "SlowmoStepAmount").GetFloat();

    Configuration::Write();

    if (EnableTrickCutting || SlowmoDuringThrow) {
        setenv("disable_ss_upload", "1", true);
        disableScores = true;
        if (!processScoreHooked) {
            INSTALL_HOOK_OFFSETLESS(SoloFreePlayFlowCoordinator_ProcessScore,
                il2cpp_utils::FindMethodUnsafe("", "SoloFreePlayFlowCoordinator", "ProcessScore", 3));
            processScoreHooked = true;
        }
    } else {
        disableScores = false;
    }
    log(DEBUG, "disable_ss_upload: %s", getenv("disable_ss_upload"));
}
