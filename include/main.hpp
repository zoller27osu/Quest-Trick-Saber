#pragma once

#include <dlfcn.h>

#include <unordered_set>

#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/utils/typedefs.h"

// Beatsaber update settings
// Whether the trail follows the Saber component, or the saber model.
constexpr bool TrailFollowsSaberComponent = true;
// Whether to attach a TrickModel for spin
extern bool AttachForSpin;
// Whether the Spin trick spins the VRController, or relative to it.
extern bool SpinIsRelativeToVRController;

inline ModInfo modInfo;
const Logger& logger();

inline std::unordered_set<Il2CppObject*> fakeSabers;
inline std::unordered_set<Il2CppReflectionType*> tBurnTypes;

void DisableBurnMarks(int saberType);
void EnableBurnMarks(int saberType);

extern "C" void load();
