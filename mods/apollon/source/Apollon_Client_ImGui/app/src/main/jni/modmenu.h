// modmenu.h -- LeviLaunchroid Mod Menu registration for Apollon.
//
// Replaces the ImGui surface, which could never appear because its only frame
// pump was a JNI entry point the game never called (Levi dlopen()s the mod and
// does not run JNI_OnLoad). See modmenu.cpp for the full explanation.
//
// Returns the number of modules registered; 0 means the menu API was
// unreachable (libpreloader.so missing or an older launcher).
#pragma once

// Implemented in main.cpp. Re-applies every toggleable hex patch from the
// current feature flags. Called once at startup (from hack_thread, after the
// patch objects exist) and again on every Mod Menu toggle, because there is no
// per-frame tick hook available in this build to poll the flags.
void applyPatches();

int ApollonRegisterModMenu();
