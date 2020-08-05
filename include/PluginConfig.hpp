#pragma once

#include "main.hpp"
#include "ConfigEnums.hpp"

#include <string>
#include <unordered_map>

#include "extern/beatsaber-hook/shared/config/config-utils.hpp"


class PluginConfig {
  private:
	typedef std::unordered_map<std::string, rapidjson::Value> ConfigDefaultsMap;
	static ConfigDefaultsMap InitializeConfigDefaults() {
		ConfigDefaultsMap def;
		def.emplace("regenerateConfig", false);
		def.emplace("TriggerAction", "Throw");
		def.emplace("GripAction", "None");
		def.emplace("ThumbstickAction", "Spin");
		def.emplace("ReverseTrigger", false);
		def.emplace("ReverseGrip", false);
		def.emplace("ReverseThumbstick", false);
		// todo: add "Both" axis option?
		def.emplace("ThumbstickDirection", "Horizontal");
		def.emplace("TriggerThreshold", 0.8f);
		def.emplace("GripThreshold", 0.8f);
		def.emplace("ThumbstickThreshold", 0.8f);
		def.emplace("ControllerSnapThreshold", 0.3f);

		def.emplace("IsSpinVelocityDependent", false);
		def.emplace("SpinSpeed", 1.0f);
		def.emplace("SpinDirection", "Backward");
		def.emplace("ThrowVelocity", 1.0f);
		def.emplace("ReturnSpeed", 10.0f);
		def.emplace("EnableTrickCutting", false);
		def.emplace("SlowmoDuringThrow", false);
		def.emplace("SlowmoAmount", 0.2f);
		def.emplace("CompleteRotationMode", false);

		// Advanced settings
		def.emplace("VelocityBufferSize", 5);
		def.emplace("SlowmoStepAmount", 0.02f);
		return def;
	}
	static const ConfigDefaultsMap configDefaults;

	typedef std::unordered_map<std::string, EnumMap> EnumValuesMap;
	static EnumValuesMap GetEnumValuesMap() {
		EnumValuesMap vals;
		vals["TriggerAction"] = ACTION_NAMES;
		vals["GripAction"] = ACTION_NAMES;
		vals["ThumbstickAction"] = ACTION_NAMES;
		vals["ThumbstickDirection"] = THUMBSTICK_DIR_NAMES;
		vals["SpinDirection"] = SPIN_DIR_NAMES;
		return vals;
	}
	static const EnumValuesMap enumValues;

	static void AddDefault(ConfigDocument& config, std::string_view key) {
		auto& allocator = config.GetAllocator();
		config.AddMember(ConfigValue::StringRefType(key.data()), ConfigValue(configDefaults.at(key.data()), allocator), allocator);
	}

	static auto& GetOrAddDefault(ConfigDocument& config, std::string_view member) {
		if (!config.HasMember(member.data())) {
			logger().error("config was missing property '%s'! Adding default value to config file.", member.data());
			AddDefault(config, member);
		}
		return config[member.data()];
	}

	// TODO: add clamping method(s) for ints & floats with error messages?
	// use https://github.com/Tencent/rapidjson/blob/master/doc/pointer.md ?
	// OR switch to the normie Config-class-yeet-your-changes way?

	static int ValidatedEnumMember(ConfigDocument& config, std::string member) {
		auto* val = GetOrAddDefault(config, member).GetString();
		auto& possible = enumValues.at(member.c_str());
		CRASH_UNLESS(possible.size());
		if (!possible.count(val)) {
			logger().error("config had invalid value '%s' for property '%s'! Setting to default.", val, member.data());
			config[member.data()] = ConfigValue(configDefaults.at(member.c_str()), config.GetAllocator());
			val = config[member.data()].GetString();
		}
		return possible.at(val);
	}

	// this method should never need to be changed
	static void CheckRegenConfig(Configuration& config) {
		auto& doc = config.config;
		if (!doc.HasMember("regenerateConfig") || doc["regenerateConfig"].GetBool()) {
			logger().warning("Regenerating config!");
			doc.SetObject();
			for (auto&& p : configDefaults) {
				AddDefault(doc, p.first);
			}
			config.Write();
			logger().info("Config regeneration complete!");
		} else {
			logger().info("Not regnerating config.");
		}
	}

	PluginConfig() {}

  public:
  	PluginConfig(const PluginConfig&) = delete;
	PluginConfig& operator=(const PluginConfig&) = delete;
    static PluginConfig& Instance() {
		static PluginConfig instance;
		return instance;
	}

	static Configuration& config() {
		static Configuration config(modInfo);
		return config;
	}

	static void Init() {
		config().Load();
		CheckRegenConfig(config());
	}

	// config fields
	TrickAction TriggerAction;
	TrickAction GripAction;
	TrickAction ThumbstickAction;
	bool ReverseTrigger;
	bool ReverseGrip;
	bool ReverseThumbstick;
	ThumbstickDir ThumbstickDirection;
	float TriggerThreshold;
	float GripThreshold;
	float ThumbstickThreshold;

	bool IsVelocityDependent;
	float SpinSpeed;
	SpinDir SpinDirection;
	float ControllerSnapThreshold;
	float ThrowVelocity;
	bool EnableTrickCutting;
	bool CompleteRotationMode;
	float ReturnSpeed;
	bool SlowmoDuringThrow;
	float SlowmoAmount;

	int VelocityBufferSize;
	float SlowmoStepAmount;

	void Reload();
};
