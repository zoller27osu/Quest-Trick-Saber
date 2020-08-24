#pragma once

#include <dlfcn.h>

#include "modloader/shared/modloader.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"

static ModInfo modInfo;
const Logger& logger();

void DisableBurnMarks(int saberType);
void EnableBurnMarks(int saberType);

extern "C" void load();
