#include "../include/main.hpp"




extern "C" void load() {
    log(INFO, "Hello from il2cpp_init!");
    log(INFO, "Installing hooks...");
    INSTALL_HOOK_OFFSETLESS(GameplayModifierToggle_Start, il2cpp_utils::FindMethodUnsafe("", "GameplayModifierToggle", "Start", 0));
    log(INFO, "Installed all hooks!");
}